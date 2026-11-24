# Task 2: Command-Line Interface Enhancements

**Status:** Pending  
**Priority:** High  
**Complexity:** Medium  
**Sessions:** 1  
**Dependencies:** Task 1 (Config enhancements)

---

## Objective

Add CLI flags for dry-run, verbosity control, and JSON output. These are additive features (not breaking) but lay groundwork for CI integration.

---

## New CLI Flags

| Flag | Alias | Purpose | Behavior |
|------|-------|---------|----------|
| `-d, --dry-run` | — | Preview sync without pushing | Parse config, log what would happen, exit 0 |
| `-vv, --verbose` | — | Increase verbosity | Set log level to DEBUG |
| `-q, --quiet` | — | Decrease verbosity | Set log level to ERROR |
| `--json` | — | Structured output for CI | Output results as JSON on stdout |
| `--help` | `-h` | Show help | Display all flags and exit (exists, verify) |
| `--version` | `-V` | Show version | Display version and exit (exists, verify) |

---

## Detailed Requirements

### 1. Flag Parsing

**Current state:** Ad-hoc parsing in `main.cpp`

**Changes needed:**
- Refactor into a small arg parser (or use a library like `CLI11` if justifiable)
- Support short and long forms (`-d` / `--dry-run`)
- Support combined short flags (`-vvq` = verbose + quiet? or error?)
- Reject unknown flags with error message

**Recommendation:** Manual parsing (no new dependency); ~100 LOC.

### 2. Dry-Run Mode

**Flag:** `-d, --dry-run`

**Behavior:**
1. Load config normally
2. Initialize services (ProcessRunner, GitSyncService)
3. Do NOT call `git_sync_service->run()`
4. Instead, **simulate** what would be synced:
   - Log current branch
   - Log source remote URL
   - Log each target remote and URL
   - Log which branches/remotes would be synced (after filtering)
   - **Do not push**
5. Exit with status 0 (success)

**Example output (non-JSON):**
```
[INFO] Running in dry-run mode (no changes will be made)
[DEBUG] Current branch: feature/new-feature
[DEBUG] Source remote: origin
[DEBUG] Source URL: git@github.com:user/repo.git
[DEBUG] Will sync to remotes:
[DEBUG]   - gitlab: git@gitlab.com:user/backup.git
[DEBUG]   - github: git@github.com:user/mirror.git
[DEBUG] Will exclude branches: [main, master]
[DEBUG] Will exclude remotes: [upstream, base]
[INFO] Dry-run complete (exit code: 0)
```

### 3. Verbosity Control

**Flags:** `-vv, --verbose` and `-q, --quiet`

**Override precedence (high to low):**
1. CLI flags (`-vv` overrides config)
2. Config file (`log_level`)
3. Default (`"info"`)

**Log levels:**
- `trace` ← (via repeated `-vv`?)
- `debug` ← `-vv`
- `info` ← default
- `warn`
- `error` ← `-q`
- `critical`

**Behavior:**
- `-vv` sets level to DEBUG (enable detailed logging)
- `-q` sets level to ERROR (suppress info, only show errors)
- Both together: undefined (error or last-one-wins)

### 4. JSON Output Format

**Flag:** `--json`

**Output schema:**
```json
{
  "status": "success" | "partial" | "error",
  "dry_run": false,
  "exit_code": 0,
  "summary": {
    "success": 2,
    "failed": 0,
    "total": 2
  },
  "pushed_to": ["gitlab", "github"],
  "failed": [],
  "metadata": {
    "timestamp": "2026-11-24T12:30:00Z",
    "duration_ms": 1234,
    "version": "1.0.0"
  }
}
```

**Behavior:**
- Structured output on **stdout** (clean for `jq` parsing)
- Logs still go to **stderr** (if log level permits)
- Valid JSON even on partial failures

**Examples:**

**Success:**
```json
{
  "status": "success",
  "dry_run": false,
  "exit_code": 0,
  "summary": {"success": 2, "failed": 0, "total": 2},
  "pushed_to": ["gitlab", "github"],
  "failed": [],
  "metadata": {"timestamp": "2026-11-24T12:30:00Z", "duration_ms": 500, "version": "1.0.0"}
}
```

**Partial (1 of 2 remotes failed):**
```json
{
  "status": "partial",
  "dry_run": false,
  "exit_code": 2,
  "summary": {"success": 1, "failed": 1, "total": 2},
  "pushed_to": ["gitlab"],
  "failed": [{"remote": "github", "error": "ssh: permission denied"}],
  "metadata": {"timestamp": "2026-11-24T12:30:00Z", "duration_ms": 2000, "version": "1.0.0"}
}
```

**Error (config error):**
```json
{
  "status": "error",
  "dry_run": false,
  "exit_code": 3,
  "summary": null,
  "pushed_to": [],
  "failed": [],
  "error": "Config error: missing required field 'remote_urls'",
  "metadata": {"timestamp": "2026-11-24T12:30:00Z", "duration_ms": 50, "version": "1.0.0"}
}
```

---

## Implementation Plan

### Phase 1: Refactor CLI Argument Parsing

**Files:**
- `src/main.cpp`
- `include/aw/gitsync/config_reader.hpp` (add CLI args struct)

**Changes:**
1. Create `struct CliArgs` with parsed flags
2. Write `CliArgs parse_args(int argc, char* argv[])`
3. Test each flag independently

**Example structure:**
```cpp
struct CliArgs {
    bool dry_run = false;
    bool verbose = false;
    bool quiet = false;
    bool json_output = false;
    bool show_help = false;
    bool show_version = false;
};
```

### Phase 2: Implement Dry-Run Mode

**Files:**
- `src/git_sync_service.cpp`
- `include/aw/gitsync/git_sync_service.hpp`

**Changes:**
1. Add `void GitSyncService::dry_run()` method
2. Log what would be synced (branches, remotes, URLs)
3. Don't call actual git commands
4. Update `main.cpp` to call `dry_run()` instead of `run()` when flag set

### Phase 3: Implement Verbosity Control

**Files:**
- `src/main.cpp`
- `src/config_reader.cpp`

**Changes:**
1. Apply CLI verbosity flags to override config log_level
2. Set spdlog level before config is processed
3. Test precedence (CLI > config > default)

### Phase 4: Implement JSON Output

**Files:**
- `src/git_sync_service.cpp`
- `include/aw/gitsync/git_sync_service.hpp`
- `src/main.cpp`

**Changes:**
1. Create `struct SyncResult` with status, summary, pushed_to, failed
2. Add method `SyncResult GitSyncService::get_last_result()`
3. Add method to serialize `SyncResult` to JSON
4. Output JSON to stdout in `main()` if `--json` flag set
5. Logs continue to stderr

### Phase 5: Update Help Message

**File:**
- `src/main.cpp`

**Add:**
```
Usage: gitsync [OPTIONS]

OPTIONS:
  -d, --dry-run      Preview sync without pushing
  -vv, --verbose     Increase verbosity (DEBUG level)
  -q, --quiet        Decrease verbosity (ERROR level)
  --json             Output results as JSON for CI
  -h, --help         Show this help message
  -V, --version      Show version and exit

ENVIRONMENT:
  GITSYNC_CONFIG_PATH  Override config file path

EXAMPLES:
  gitsync --dry-run              Preview what would be synced
  gitsync -vv                    Run with DEBUG logging
  gitsync --json                 Output as JSON
  gitsync -d --json              Dry-run with JSON output
```

### Phase 6: Tests

**File:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Parse each flag individually
2. Parse combined flags
3. Dry-run doesn't modify git state
4. JSON output is valid JSON and contains all fields
5. Verbosity overrides config
6. Help and version messages display

---

## Acceptance Criteria

- ✅ All CLI flags implemented and tested
- ✅ Flags can be combined sensibly
- ✅ Dry-run shows preview without side effects
- ✅ Verbosity flags override config file settings
- ✅ JSON output is valid and contains required fields
- ✅ Help message is clear and complete
- ✅ No breaking changes to existing behavior
- ✅ Exit code is correct for JSON output

---

## Notes

- JSON output is key for CI integration (Task 6 builds on this)
- Dry-run is a safeguard for new users (critical feature)
- Verbosity is nice-to-have but useful for debugging (low complexity, high value)

