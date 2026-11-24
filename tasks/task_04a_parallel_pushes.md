# Task 4a: Parallel Remote Pushes

**Status:** Pending  
**Priority:** Medium  
**Complexity:** High  
**Sessions:** 1  
**Dependencies:** Task 2 (CLI flags framework)

---

## Objective

Add parallel push capability. By default, pushes are sequential (as today). With `--jobs=N` flag, push to N remotes concurrently for faster syncs.

---

## Requirements

### 1. Command-Line Flag

**New flag:** `--jobs=N` (or `-j N`)
- Default: 1 (sequential, as today)
- Valid range: 1–32 (or system CPU count)
- Example: `gitsync --jobs=4` (push to 4 remotes in parallel)

### 2. Thread Pool Implementation

**Options:**
- A. Manual thread creation (start/join threads)
- B. Use `std::async` with `std::future`
- C. Add a thread pool library (e.g., `BS::thread_pool`)

**Recommendation:** Option B (`std::async`) for simplicity and no extra dependencies.

### 3. Concurrent Push Logic

**Current (sequential):**
```
for each remote:
  push_remote(name, refspec)  // blocks until done
  record result
```

**New (parallel):**
```
futures = []
for each remote:
  future = async(push_remote, name, refspec)
  futures.push(future)

for each future:
  wait for result
  record result
```

### 4. Result Aggregation

**Behavior:**
1. Start all pushes
2. Wait for all to complete
3. Collect results
4. Compute summary (success/failed counts)
5. Log results

### 5. Error Handling

**Thread safety:**
- Avoid race conditions on shared state (config, process_runner)
- Each thread works independently
- Results collected after all threads complete

**Logging:**
- Each thread logs to spdlog (spdlog is thread-safe)
- Final summary is computed serially before output

---

## Implementation Plan

### Phase 1: Add `--jobs` Flag

**Files:**
- `src/main.cpp`
- `include/aw/gitsync/config_reader.hpp` (extend CliArgs)

**Changes:**
1. Parse `--jobs=N` from command line
2. Validate range (1–32 or CPU count)
3. Store in `CliArgs` struct
4. Pass to `GitSyncService`

### Phase 2: Extend GitSyncService

**Files:**
- `include/aw/gitsync/git_sync_service.hpp`
- `src/git_sync_service.cpp`

**Changes:**
1. Add member: `uint32_t job_count_` (default 1)
2. Add setter: `void set_job_count(uint32_t n)`
3. Add method: `std::vector<std::pair<std::string, bool>> push_all_parallel(const std::string& refspec, uint32_t jobs)`
4. Modify `run()` to call parallel version if `job_count_ > 1`

### Phase 3: Implement Async Push

**Files:**
- `src/git_sync_service.cpp`

**New method:**
```cpp
std::vector<std::pair<std::string, bool>>
GitSyncService::push_all_parallel(const std::string& refspec, uint32_t jobs) {
    std::vector<std::pair<std::string, bool>> results;
    std::vector<std::future<bool>> futures;
    
    // Start async pushes
    for (const auto& [name, url] : config_.remote_urls) {
        futures.emplace_back(
            std::async(std::launch::async, [this, name, refspec]() {
                try {
                    push_remote(name, refspec);
                    return true;
                } catch (const std::exception&) {
                    return false;
                }
            })
        );
        
        // Limit concurrency: if N futures pending, wait for one
        if (futures.size() >= jobs) {
            // Wait for first future, collect result
            bool ok = futures[0].get();
            results.emplace_back(config_.remote_urls.cbegin()->first, ok);
            futures.erase(futures.begin());
        }
    }
    
    // Wait for remaining futures
    for (auto& future : futures) {
        future.wait();
        // ... collect result
    }
    
    return results;
}
```

**Note:** This is a simplified sketch. Real implementation needs careful result tracking.

### Phase 4: Thread Safety Review

**Files:**
- `src/process_runner.cpp`
- `src/git_sync_service.cpp`

**Checks:**
1. Verify `ProcessRunner::run()` is thread-safe (fork/popen is per-thread safe)
2. Verify `spdlog` calls are thread-safe (they are)
3. Verify `config_` is not modified during pushes (it's read-only)
4. Test with sanitizers (ThreadSanitizer)

### Phase 5: Tests

**File:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Parse `--jobs` flag
2. Single-threaded vs multi-threaded give same results (order may differ)
3. Test with 2, 4, 8 job counts
4. Verify all remotes are pushed even with failures
5. Performance comparison (optional): time single vs parallel

---

## Acceptance Criteria

- ✅ `--jobs=N` flag implemented
- ✅ Parallel pushes work without data races
- ✅ Results aggregated correctly regardless of job count
- ✅ ThreadSanitizer passes (no race conditions)
- ✅ Performance improves with parallelism
- ✅ Backward compatible (default is sequential)
- ✅ All tests pass

---

## Performance Notes

**Expected speedup:**
- Syncing to 4 remotes with 1 second per push:
  - Sequential: 4 seconds
  - Parallel (--jobs=4): ~1 second (depends on network)

**Diminishing returns:**
- Beyond CPU count / network concurrency limit, no benefit
- Safe default: 1 (sequential)

---

## Example Usage

```bash
# Sync to 2 remotes in parallel
gitsync --jobs=2

# Sync to 4 remotes in parallel (or fewer if config has fewer)
gitsync --jobs=4

# Dry-run with parallel simulation
gitsync --dry-run --jobs=2

# JSON output with parallel pushes
gitsync --json --jobs=4
```

