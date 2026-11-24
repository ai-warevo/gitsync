# Task 4b: Retry Logic

**Status:** Pending  
**Priority:** High  
**Complexity:** Medium  
**Sessions:** 1  
**Dependencies:** Task 1 (Config field `max_retries`)

---

## Objective

Implement automatic retry logic for failed pushes. Configuration field `max_retries` controls how many retries are attempted after the first failure, with exponential backoff between attempts.

---

## Terminology

**Important:** Clarify retry semantics

- `max_retries = 1` means:
  - **1 initial attempt** + **1 retry** = **2 total requests**
  - If first fails, automatically try again once
  
- `max_retries = 0` means:
  - **1 initial attempt** only
  - No retries, fail immediately
  
- `max_retries = 2` means:
  - **1 initial attempt** + **2 retries** = **3 total requests**

---

## Requirements

### 1. Configuration

**Config field (from Task 1):**
- `max_retries`: Integer, default: 1

**Scope:**
- Applied globally (all remotes use same retry count)
- Can be overridden at org/repo level

### 2. Retry Logic

**Behavior per remote push:**
1. Attempt 1: Try push
2. If fails and `attempt < max_retries`, wait then retry
3. After `max_retries` total attempts, give up

**Backoff strategy:** Exponential backoff
- Wait time = 2^(attempt - 1) seconds
- Attempt 1: fail → wait 2^0 = 1 second → Attempt 2
- Attempt 2: fail → wait 2^1 = 2 seconds → Attempt 3
- Attempt 3: fail → give up

**Example with `max_retries = 1`:**
```
Attempt 1: push to gitlab → SSH timeout
  Wait 1 second
Attempt 2: push to gitlab → Success
Result: Success (after 1 retry)
```

**Example with `max_retries = 2`:**
```
Attempt 1: push to github → Network error
  Wait 1 second
Attempt 2: push to github → Network error
  Wait 2 seconds
Attempt 3: push to github → Permission denied
Result: Failed (all retries exhausted)
```

### 3. Logging

**Log each attempt:**
- At DEBUG level, log attempt number and reason for failure
- At WARN level, log final failure after retries exhausted

**Example log output:**
```
[DEBUG] Pushing to gitlab (attempt 1/2)
[WARN] Push failed: ssh: permission denied. Retrying in 1 second...
[DEBUG] Pushing to gitlab (attempt 2/2)
[INFO] Push succeeded
```

### 4. Error Classification

**Retryable errors** (attempt again):
- Network timeouts
- Temporary SSH errors
- DNS resolution failures
- 5xx server errors

**Non-retryable errors** (fail immediately):
- SSH key not found
- Repository not found (404)
- Permission denied (403)
- Invalid commit

**Heuristic:** Currently, retry on any error. Refine later if needed.

---

## Implementation Plan

### Phase 1: Update GitSyncService Interface

**Files:**
- `include/aw/gitsync/git_sync_service.hpp`
- `src/git_sync_service.cpp`

**Changes:**
1. Add member: `uint32_t max_retries_` (from config)
2. Add method: `bool push_remote_with_retry(const std::string& name, const std::string& refspec)`
3. Keep `push_remote()` for single attempt (used internally or by tests)

### Phase 2: Implement Retry Loop

**Files:**
- `src/git_sync_service.cpp`

**New method:**
```cpp
bool GitSyncService::push_remote_with_retry(const std::string& name,
                                            const std::string& refspec) {
    for (uint32_t attempt = 1; attempt <= (1 + max_retries_); ++attempt) {
        try {
            spdlog::debug("Pushing to {} (attempt {}/{})",
                         name, attempt, 1 + max_retries_);
            push_remote(name, refspec);
            return true;  // Success
        } catch (const std::exception& e) {
            if (attempt < 1 + max_retries_) {
                uint32_t wait_secs = 1u << (attempt - 1);  // 2^(attempt-1)
                spdlog::warn("Push failed: {}. Retrying in {} seconds...",
                            e.what(), wait_secs);
                std::this_thread::sleep_for(
                    std::chrono::seconds(wait_secs));
            } else {
                spdlog::error("Push failed after {} attempts: {}",
                             1 + max_retries_, e.what());
                return false;  // All retries exhausted
            }
        }
    }
    return false;  // Should not reach here
}
```

### Phase 3: Update Sync Flow

**Files:**
- `src/git_sync_service.cpp`

**Changes:**
1. In `push_all()`, call `push_remote_with_retry()` instead of `push_remote()`
2. In `push_all_parallel()`, each async task calls `push_remote_with_retry()`

### Phase 4: Backoff Delay Implementation

**Files:**
- `src/git_sync_service.cpp`

**Options for delay:**
- A. `std::this_thread::sleep_for()` — blocks current thread
- B. `std::chrono::high_resolution_clock` — for precision

**Recommendation:** Option A (simple, works in both sequential and async contexts)

**Note:** In parallel mode, each thread sleeps independently, so delays don't block other pushes.

### Phase 5: Tests

**File:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Test retry succeeds on 2nd attempt
2. Test retry gives up after max_retries exhausted
3. Test backoff timing (verify delays are roughly exponential)
4. Test successful push on 1st attempt (no retry delay)
5. Test with mock process runner that fails N times

**Mock setup:**
```cpp
// Mock ProcessRunner that fails first N times, then succeeds
class FailingProcessRunner : public IProcessRunner {
public:
    FailingProcessRunner(uint32_t fail_count) : fail_count_(fail_count) {}
    Result run(const std::string& cmd, const std::vector<std::string>& args) override {
        if (call_count_ < fail_count_) {
            call_count_++;
            throw std::runtime_error("Simulated failure");
        }
        call_count_++;
        return {true, "", ""};
    }
private:
    uint32_t fail_count_;
    uint32_t call_count_ = 0;
};
```

---

## Acceptance Criteria

- ✅ `max_retries` config field read and applied
- ✅ Retry loop implemented with correct attempt count
- ✅ Exponential backoff timing correct (2^(attempt-1) seconds)
- ✅ Logging clear for each attempt and final result
- ✅ Works in both sequential and parallel modes
- ✅ All tests pass
- ✅ Backward compatible (default: 1 retry)

---

## Examples

### Config: No retries

```json
{
  "max_retries": 0
}
```

**Behavior:** Single attempt, fail immediately on error.

### Config: 2 retries

```json
{
  "max_retries": 2
}
```

**Behavior:** Try up to 3 times (1 + 2), wait 1 sec after 1st failure, 2 secs after 2nd.

### Config per org

```json
// Global: Lenient (2 retries)
{ "max_retries": 2 }

// Production org: Strict (0 retries)
{ "max_retries": 0 }
```

---

## Performance Considerations

**Worst case:**
- `max_retries = 2`, all pushes fail
- Per remote: 1 + 1 + 2 = 4 seconds
- 4 remotes × 4 seconds = 16 seconds total (sequential)
- 4 remotes with `--jobs=4`: ~4 seconds (parallel)

**Typical case:**
- Most pushes succeed on first attempt
- Retries only on transient failures
- Minimal overhead

---

## Related: Exit Codes

**Retry logic affects exit codes (Task 5):**
- If all retries fail for some remotes → exit code 2 (partial success)
- If all retries fail for all remotes → exit code 4 (git error) or 1 (general error)

