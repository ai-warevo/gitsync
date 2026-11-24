# Task 6: JSON Output & Permission Check (Combined)

**Status:** Pending  
**Priority:** Medium  
**Complexity:** Low  
**Sessions:** 0.5  
**Dependencies:** Task 2 (JSON output format defined), Task 1 (Config system)

---

## Objective

**Two low-complexity features combined:**
1. Implement full JSON output with error details for CI integration
2. Add warning when config file is world-readable (security issue)

---

## Part 1: JSON Output Format for CI

### Requirements

**Flag:** `--json` (from Task 2)

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

### Full Examples

**Success (all remotes):**
```json
{
  "status": "success",
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
    "timestamp": "2026-11-24T12:30:00.123Z",
    "duration_ms": 523,
    "version": "1.0.0"
  }
}
```

**Partial (1 of 2 failed):**
```json
{
  "status": "partial",
  "dry_run": false,
  "exit_code": 2,
  "summary": {
    "success": 1,
    "failed": 1,
    "total": 2
  },
  "pushed_to": ["gitlab"],
  "failed": [
    {
      "remote": "github",
      "error": "ssh: permission denied (publickey)",
      "attempts": 2
    }
  ],
  "metadata": {
    "timestamp": "2026-11-24T12:30:00.456Z",
    "duration_ms": 2345,
    "version": "1.0.0"
  }
}
```

**Error (config problem):**
```json
{
  "status": "error",
  "dry_run": false,
  "exit_code": 3,
  "summary": null,
  "pushed_to": [],
  "failed": [],
  "error": "Config error: invalid JSON in ~/.gitsync/config.json at line 5",
  "metadata": {
    "timestamp": "2026-11-24T12:30:00.789Z",
    "duration_ms": 50,
    "version": "1.0.0"
  }
}
```

**Dry-run (no actual push):**
```json
{
  "status": "success",
  "dry_run": true,
  "exit_code": 0,
  "summary": {
    "success": 0,
    "failed": 0,
    "total": 0
  },
  "pushed_to": [],
  "failed": [],
  "would_push_to": ["gitlab", "github"],
  "filtered_branches": ["feature/new", "bugfix/old"],
  "filtered_remotes": ["gitlab", "github"],
  "metadata": {
    "timestamp": "2026-11-24T12:30:00.901Z",
    "duration_ms": 234,
    "version": "1.0.0"
  }
}
```

### Implementation Plan

**Files:**
- `src/git_sync_service.cpp`
- `include/aw/gitsync/git_sync_service.hpp`
- `src/main.cpp`

**Changes:**

1. **Create result struct:**
```cpp
struct SyncResult {
    std::string status;  // "success", "partial", "error"
    bool dry_run;
    int exit_code;
    
    struct Summary {
        uint32_t success;
        uint32_t failed;
        uint32_t total;
    };
    std::optional<Summary> summary;
    
    std::vector<std::string> pushed_to;
    std::vector<std::pair<std::string, std::string>> failed;  // {remote, error}
    std::optional<std::string> error;
    
    nlohmann::json to_json() const;
};
```

2. **Collect results during sync:**
   - Track each remote push result
   - Store error message if failed
   - Track number of retry attempts

3. **Serialize to JSON:**
   - Include timestamp, duration, version metadata
   - Format duration as milliseconds
   - Use ISO 8601 timestamp format

4. **Output in main():**
   - If `--json` flag set, output JSON to stdout
   - Logs always go to stderr
   - Valid JSON even on errors

---

## Part 2: Config File Permissions Check

### Requirements

**Security issue:**
- If `.gitsync/config.json` is world-readable, it may expose mirror URLs with credentials
- Warn user and suggest fix

**What to check:**
- File mode bits: readable by others means `(mode & 0o004) != 0`
- Example: `-rw-r--r--` (644) is world-readable ❌
- Example: `-rw-------` (600) is safe ✅

**Platform support:**
- Linux/macOS: Use `std::filesystem::status()` to check permissions
- Windows: Note that file permissions work differently; may skip check

### Logging

**At config load time:**
- If world-readable, log WARNING
- Suggest: `chmod 600 ~/.gitsync/config.json`
- Continue execution (don't block)

**Example log:**
```
[WARN] Config file is world-readable: ~/.gitsync/config.json (mode 644)
[WARN] Recommendation: chmod 600 ~/.gitsync/config.json
```

### Implementation Plan

**Files:**
- `src/config_reader.cpp`
- `include/aw/gitsync/utils.hpp` (optional)

**Changes:**

1. **Add permission check utility:**
```cpp
#include <filesystem>

bool is_file_world_readable(const std::filesystem::path& path) {
    auto perms = std::filesystem::status(path).permissions();
    // Check if "other" has read permission
    return (perms & std::filesystem::perms::others_read) != std::filesystem::perms::none;
}

void warn_if_world_readable(const std::filesystem::path& config_path) {
    if (is_file_world_readable(config_path)) {
        spdlog::warn("Config file is world-readable: {} (may expose credentials)",
                    config_path.string());
        spdlog::warn("Recommendation: chmod 600 {}", config_path.string());
    }
}
```

2. **Call after each config file load:**
   - In `ConfigReader::read_config()`, after each file is loaded
   - Call for global, org, and repo config files

3. **Windows consideration:**
   - On Windows, file permissions are handled differently (ACLs)
   - May skip check or provide different warning
   - Use `#ifdef _WIN32` guard

---

## Acceptance Criteria

**JSON Output:**
- ✅ Output is valid JSON
- ✅ All expected fields present
- ✅ Error details included for failed remotes
- ✅ Metadata (timestamp, duration, version) included
- ✅ Works with --dry-run, --json flags
- ✅ Works with retry logic (attempt count in failed details)

**Permission Check:**
- ✅ Detects world-readable config files
- ✅ Logs warning with suggestion
- ✅ Execution continues (doesn't block)
- ✅ Works on Linux, macOS, Windows (or notes limitations)

---

## Testing

**JSON Output Tests:**
```cpp
TEST_CASE("JSON output format") {
    auto result = run_gitsync_with_json_flag();
    auto json = nlohmann::json::parse(result);
    REQUIRE(json.contains("status"));
    REQUIRE(json.contains("exit_code"));
    REQUIRE(json.contains("metadata"));
}

TEST_CASE("JSON output on partial failure") {
    // Mock 1 success, 1 failure
    auto result = run_gitsync_partial_failure_json();
    auto json = nlohmann::json::parse(result);
    REQUIRE(json["status"] == "partial");
    REQUIRE(json["summary"]["failed"] == 1);
    REQUIRE(json["failed"].size() == 1);
}
```

**Permission Check Tests:**
```cpp
TEST_CASE("Permission check warns") {
    create_world_readable_config();
    auto logs = run_gitsync_capture_logs();
    REQUIRE(logs.find("world-readable") != std::string::npos);
}

TEST_CASE("Permission check doesn't block execution") {
    create_world_readable_config();
    auto exit_code = run_gitsync();
    REQUIRE(exit_code == 0);  // Still succeeds
}
```

---

## Example Usage

**JSON output:**
```bash
$ gitsync --json
{
  "status": "success",
  "dry_run": false,
  "exit_code": 0,
  "summary": {"success": 2, "failed": 0, "total": 2},
  "pushed_to": ["gitlab", "github"],
  "failed": [],
  "metadata": {"timestamp": "2026-11-24T12:30:00Z", "duration_ms": 523, "version": "1.0.0"}
}
```

**Dry-run with JSON:**
```bash
$ gitsync --dry-run --json
{
  "status": "success",
  "dry_run": true,
  "exit_code": 0,
  "would_push_to": ["gitlab", "github"],
  "filtered_branches": ["feature/new", "bugfix/old"],
  "filtered_remotes": ["gitlab", "github"],
  ...
}
```

**Permission warning:**
```bash
$ gitsync
[WARN] Config file is world-readable: ~/.gitsync/config.json (mode 644)
[WARN] Recommendation: chmod 600 ~/.gitsync/config.json
[INFO] Start syncing
...
```

---

## Notes

- JSON output integrates with exit codes (Task 5) — exit_code in JSON matches shell $?
- Permission check is advisory only — doesn't prevent sync, just warns
- Both features are low-complexity additions

