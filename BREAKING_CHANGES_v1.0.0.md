# gitsync v1.0.0 Breaking Changes Release Plan

## Overview
This document outlines the breaking changes and new features for gitsync v1.0.0. The release is split across multiple sessions to manage scope and complexity. Each session focuses on specific features with clear acceptance criteria.

---

## Task 1: Configuration Format Enhancements (YAML Support + Field Rename)

**Goal:** Support both JSON and YAML config formats; migrate PascalCase → snake_case  
**Breaking Change:** Yes (config field names change)  
**Sessions Needed:** 1  
**Complexity:** High (config system rewrite)

### Requirements

1. **Add YAML support** via `nlohmann/json` YAML integration (or lightweight YAML parser)
   - Auto-detect format by file extension (`.json` vs `.yaml`/`.yml`)
   - Fallback: Try JSON first, then YAML if JSON parsing fails
   
2. **Support both PascalCase and snake_case field names (dual convention):**
   - `LogLevel` / `log_level` — both accepted
   - `SourceRemoteName` / `source_remote_name` — both accepted
   - `RemoteBranchTemplate` / `remote_branch_template` — both accepted
   - `RemoteUrls` / `remote_urls` — both accepted
   - `SshKeyTypes` / `ssh_key_types` — both accepted
   - If both present, snake_case takes precedence
   - **NEW:** `max_retries` (integer, default: 1)
   - **NEW:** `include_branches` (string array, default: `["*"]` = all)
   - **NEW:** `exclude_branches` (string array, default: `[]`)
   - **NEW:** `include_remotes` (string array, default: `["*"]` = all)
   - **NEW:** `exclude_remotes` (string array, default: `["upstream", "base"]`)

3. **Migration guide (optional):**
   - Existing PascalCase configs work without changes
   - Users can optionally migrate to snake_case at their own pace
   - New projects should use snake_case convention
   - No pressure, no breaking changes

4. **Testing:**
   - Load sample YAML configs, verify merge behavior
   - Load sample JSON configs, verify snake_case parsing
   - Test config hierarchy (global → org → repo)
   - Test new filter fields (branches, remotes)

### Files to Modify/Create
- `include/aw/gitsync/config_settings.hpp` — Add new fields
- `src/config_settings.cpp` — Update merge logic
- `src/config_reader.cpp` — Add YAML parsing, file type detection
- `CMakeLists.txt` — Add YAML parser dependency (if needed)
- `docs/README.md` — Update config docs with examples
- `tests/gitsync_tests.cpp` — Add config loading tests

### Exit Criteria
- ✅ All config fields renamed to snake_case across codebase
- ✅ YAML files load and merge correctly
- ✅ Config hierarchy works with both JSON and YAML
- ✅ New filter fields stored (not yet applied to sync logic)
- ✅ All existing tests pass
- ✅ Config docs updated with examples

---

## Task 2: Command-Line Interface Enhancements

**Goal:** Add flags for --dry-run, --verbose, --quiet, --json  
**Breaking Change:** No (additive)  
**Sessions Needed:** 1  
**Complexity:** Medium

### Requirements

1. **New flags:**
   - `-d, --dry-run` — Parse config, show what would be synced, exit without pushing
   - `-vv, --verbose` — Set log level to DEBUG (or higher verbosity)
   - `-q, --quiet` — Set log level to ERROR
   - `--json` — Output results in JSON format for CI integration
   - `--help, -h` — Show help message (already exists, verify)
   - `--version, -V` — Show version (already exists, verify)

2. **CLI argument parsing:**
   - Migrate from ad-hoc parsing to a small arg parser (e.g., manual parsing with cleaner structure)
   - Flags override config file log_level
   - `--dry-run` doesn't mutate git state

3. **Output format for `--json`:**
   ```json
   {
     "status": "success" | "partial" | "error",
     "dry_run": false,
     "pushed_to": ["remote1", "remote2"],
     "failed": ["remote3"],
     "summary": {
       "success": 2,
       "failed": 1,
       "total": 3
     },
     "exit_code": 0
   }
   ```

4. **Testing:**
   - Verify each flag works in isolation and combined
   - Test `--dry-run` doesn't create remotes or push
   - Test `--json` output is valid JSON
   - Test verbosity levels

### Files to Modify/Create
- `src/main.cpp` — Enhanced CLI parsing, flag handling
- `include/aw/gitsync/config_reader.hpp` — Add CLI args struct
- `src/git_sync_service.cpp` — Add dry-run mode, JSON output format
- `tests/gitsync_tests.cpp` — CLI tests

### Exit Criteria
- ✅ All flags implemented and tested
- ✅ Flags override config file settings
- ✅ Dry-run shows what would happen without side effects
- ✅ JSON output is valid and contains all expected fields
- ✅ Help message is clear and complete

---

## Task 3: Branch & Remote Filtering

**Goal:** Implement branch/remote inclusion/exclusion logic  
**Breaking Change:** No (config additions are backward-compatible, all branches/remotes sync by default)  
**Sessions Needed:** 1  
**Complexity:** Medium

### Requirements

1. **Branch filtering:**
   - `include_branches`: Array of glob patterns (default: `["*"]` = include all)
   - `exclude_branches`: Array of glob patterns (default: `[]` = exclude none)
   - Logic: Include if matches `include_branches` AND does not match `exclude_branches`
   - Example excludes: `["main", "master", "develop", "release/*"]`

2. **Remote filtering:**
   - `include_remotes`: Array of remote names (default: `["*"]` = all)
   - `exclude_remotes`: Array of remote names (default: `["upstream", "base"]` = skip these)
   - Logic: Include if in `include_remotes` AND not in `exclude_remotes`

3. **Configuration at all three levels:**
   - Global, org, repo configs can each specify filters
   - Merge behavior: Arrays are merged/extended, not replaced

4. **Logging:**
   - Log which branches/remotes are included/excluded in DEBUG mode
   - Warn if config matches zero remotes

5. **Testing:**
   - Test glob patterns for branch names
   - Test remote inclusion/exclusion
   - Test config merge for filter arrays
   - Test edge cases (empty arrays, no matches)

### Files to Modify/Create
- `include/aw/gitsync/git_sync_service.hpp` — Add filter methods
- `src/git_sync_service.cpp` — Implement branch/remote filtering logic
- `src/config_settings.cpp` — Merge filter arrays correctly
- `tests/gitsync_tests.cpp` — Filter tests

### Exit Criteria
- ✅ Branch filtering works with glob patterns
- ✅ Remote filtering works
- ✅ Config merge handles filter arrays correctly
- ✅ Zero-match cases logged/warned
- ✅ All tests pass

---

## Task 4: Parallel Remote Pushes + Retry Logic

**Goal:** Push to multiple remotes concurrently; implement retry on failure  
**Breaking Change:** No (feature addition)  
**Sessions Needed:** 2 (1 for parallel, 1 for retry)  
**Complexity:** High

### Requirements

#### 4a. Parallel Remote Pushes
- Add `--jobs=N` flag (default: 1 = sequential, as today)
- Use thread pool or async to push to N remotes in parallel
- Collect results and report summary
- Synchronization: Ensure all pushes complete before summary

#### 4b. Retry Logic
- Config field: `max_retries` (integer, default: 1)
  - `1` = try once, if fails do 1 retry = 2 total attempts
  - `0` = no retries, fail immediately
  - `2` = 1 initial + 2 retries = 3 total attempts
- Exponential backoff: Wait 2^attempt seconds between retries
- Log each retry attempt with reason
- Final failure includes all retry logs

### Files to Modify/Create
- `src/main.cpp` — Add `--jobs` flag
- `include/aw/gitsync/git_sync_service.hpp` — Add retry/parallel methods
- `src/git_sync_service.cpp` — Implement parallel push & retry logic
- `src/process_runner.cpp` — Ensure thread-safe process execution
- `tests/gitsync_tests.cpp` — Parallel/retry tests

### Exit Criteria
- ✅ `--jobs=N` flag implemented and tested
- ✅ Parallel pushes work without data races
- ✅ Retry logic works with backoff
- ✅ Config `max_retries` is applied
- ✅ Results aggregated correctly

---

## Task 5: Exit Codes Standardization

**Goal:** Define and implement standard exit codes for different outcomes  
**Breaking Change:** Yes (changes exit codes from current behavior)  
**Sessions Needed:** 0.5  
**Complexity:** Low

### Requirements

1. **Exit codes:**
   - `0` — All remotes synced successfully
   - `1` — General error (not config or git specific)
   - `2` — Partial success (some remotes succeeded, some failed)
   - `3` — Configuration error (invalid config, missing required fields)
   - `4` — Git error (not in repo, remote mismatch, etc.)
   - `5` — Permission/access error (SSH, file perms, etc.)

2. **Mapping:**
   - ConfigReader errors → exit 3
   - GitSyncService git errors → exit 4
   - File permission errors → exit 5
   - Partial success → exit 2
   - Full success → exit 0

3. **Documentation:**
   - Add exit codes section to README.md
   - Comment exit code assignments in main.cpp

### Files to Modify/Create
- `src/main.cpp` — Map exceptions/results to exit codes
- `include/aw/gitsync/config_reader.hpp` — Custom exception types
- `src/git_sync_service.cpp` — Distinguish error types
- `README.md` — Document exit codes

### Exit Criteria
- ✅ All exit codes implemented and tested
- ✅ Documentation updated
- ✅ Error mappings are consistent

---

## Task 6: JSON Output Format for CI Integration

**Goal:** Structured output for CI pipelines  
**Breaking Change:** No (opt-in with `--json` flag)  
**Sessions Needed:** 0.5  
**Complexity:** Low

### Requirements

1. **JSON output schema (see Task 2 for format)**
2. **Include:**
   - Overall status (success/partial/error)
   - List of successful remotes
   - List of failed remotes with error messages
   - Summary statistics
   - Execution metadata (dry_run, timestamp, duration)

3. **Stdout vs stderr:**
   - JSON on stdout (clean for piping)
   - Logs still go to stderr if log_level permits

### Files to Modify/Create
- `src/git_sync_service.cpp` — JSON result struct
- `src/main.cpp` — Output JSON on demand

### Exit Criteria
- ✅ JSON output is valid
- ✅ Contains all expected fields
- ✅ Integrates with parallel/retry features

---

## Task 7: Config File Permissions Check

**Goal:** Warn if `.gitsync/config.json` is world-readable  
**Breaking Change:** No (advisory only)  
**Sessions Needed:** 0.5  
**Complexity:** Low

### Requirements

1. **Permission check:**
   - On load, check file mode
   - Warn if readable by others: `(mode & 0o004) != 0`
   - Suggest fix: `chmod 600 .gitsync/config.json`

2. **Platform support:**
   - Linux/macOS: Use `std::filesystem::status()` to check permissions
   - Windows: Check ACL (or note that Windows handles this differently)

3. **Logging:**
   - Warn level if world-readable
   - Continue execution (don't block)

### Files to Modify/Create
- `src/config_reader.cpp` — Add permission check
- `include/aw/gitsync/utils.hpp` — Add permission check utility

### Exit Criteria
- ✅ Permission check works on Linux/Windows
- ✅ Warning logged for world-readable files
- ✅ Execution continues

---

## Task 8: Stabilize Public API & Header Exports

**Goal:** Clarify public vs private APIs; version the C++ API for library users  
**Breaking Change:** Yes (some headers may become private)  
**Sessions Needed:** 1  
**Complexity:** Medium

### Requirements

1. **Public API surface:**
   - **Public (stable, versioned):** Classes/functions users should depend on
     - `ConfigSettings` — Configuration struct
     - `ConfigReader::read_config()` — Load config
     - `GitSyncService` — Main sync service (public methods only)
     - `ProcessRunner` / `IProcessRunner` — Process execution interface
   - **Internal (use at own risk):** Implementation details
     - `KnownHostsService` — Internal, not part of public API
     - `utils.hpp` — Internal utilities, may change

2. **Header organization:**
   - Create `include/aw/gitsync/public/config.hpp` (re-exports stable APIs)
   - Mark internal headers with `// INTERNAL` comment
   - Update CMake to export public headers for `find_package()`

3. **API versioning:**
   - Define `GITSYNC_API_VERSION` (e.g., "1.0.0")
   - Document breaking changes in public API separately from CLI changes
   - Add `__declspec(dllexport)` on Windows for symbols

4. **Documentation:**
   - Add `LIBRARY_USAGE.md` for C++ library consumers
   - Outline stable vs internal APIs
   - Provide example: Link as a library, use ConfigSettings + GitSyncService

5. **CMake export:**
   - Create `gitsync-config.cmake` for `find_package(gitsync)`
   - Export public target `gitsync::public` or similar

### Files to Modify/Create
- `include/aw/gitsync/public/config.hpp` — New public API header
- `include/aw/gitsync/gitsync.hpp` — Update with version/API exports
- `CMakeLists.txt` — Add CMake export configuration
- `docs/LIBRARY_USAGE.md` — New library usage guide
- All `.hpp` files — Mark public/internal

### Exit Criteria
- ✅ Public API clearly documented
- ✅ Internal APIs marked as such
- ✅ CMake exports work for `find_package(gitsync)`
- ✅ Library usage guide provided
- ✅ No binary breaking changes (only clarity)

---

## Implementation Order

**Recommended session schedule:**

1. **Session 1:** Task 1 (Config enhancements: YAML + snake_case)
2. **Session 2:** Task 2 (CLI flags: dry-run, verbose, quiet, json)
3. **Session 3:** Task 3 (Branch/remote filtering)
4. **Session 4:** Task 4a (Parallel pushes)
5. **Session 5:** Task 4b (Retry logic)
6. **Session 6:** Task 5 + 6 + 7 (Exit codes, JSON output, permissions check) — combined, low complexity
7. **Session 7:** Task 8 (Public API stabilization)

**Total:** ~7 sessions

---

## Breaking Changes Summary

For the 1.0.0 release notes, highlight:

1. **Config field names:** PascalCase → snake_case
2. **Exit codes:** Changed from 0/1 to standardized codes (0, 2, 3, 4, 5)
3. **Public API:** `aw::gitsync` namespace stable; internal APIs may change
4. **Config defaults:** `exclude_remotes` now defaults to `["upstream", "base"]` (may change sync behavior)

---

## Testing Strategy

Each task includes unit tests in `tests/gitsync_tests.cpp`. Integration tests should verify:
- Config loading from JSON and YAML
- CLI flags and their combinations
- Branch/remote filtering with real git repos (optional, in-depth tests)
- Parallel pushes and retry behavior
- Exit codes for different failure modes

