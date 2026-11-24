# Task 5: Exit Codes Standardization

**Status:** Pending  
**Priority:** High  
**Complexity:** Low  
**Sessions:** 0.5  
**Dependencies:** Tasks 1–4 (all features)

---

## Objective

Define and implement standard exit codes for different failure modes. This enables CI pipelines to respond appropriately based on the type of failure.

---

## Exit Code Reference

| Code | Meaning | Example | Action |
|------|---------|---------|--------|
| **0** | Success | All remotes synced | Continue CI/CD pipeline |
| **1** | General error | Unhandled exception | Fail CI/CD, check logs |
| **2** | Partial success | 2 of 3 remotes synced | Warn but don't block |
| **3** | Config error | Missing required field | Fix config, retry |
| **4** | Git error | Not in repo, branch mismatch | Manual intervention needed |
| **5** | Permission/access error | SSH key denied, file perms | Fix credentials/perms |

---

## Detailed Definitions

### Exit Code 0: Complete Success

**Conditions:**
- All remotes synced successfully
- No warnings or errors (or only INFO-level logs)

**Example:**
```
$ gitsync
[INFO] Start syncing
[INFO] Sync complete: 3/3 succeeded
$ echo $?
0
```

---

### Exit Code 1: General Error

**Conditions:**
- Unhandled exception
- Unexpected failure not in other categories
- Used as fallback for unknown errors

**Example:**
```
$ gitsync
[ERROR] Unexpected error: std::bad_alloc
$ echo $?
1
```

---

### Exit Code 2: Partial Success

**Conditions:**
- Some remotes synced, some failed
- Not a total loss, but incomplete
- After retries exhausted

**Example:**
```
$ gitsync --jobs=4
[INFO] Push succeeded to gitlab
[INFO] Push succeeded to github
[WARN] Push failed to backup: permission denied
[WARN] Push failed to archive: network timeout
[WARN] Sync complete: 2/4 succeeded, 2/4 errors
$ echo $?
2
```

---

### Exit Code 3: Configuration Error

**Conditions:**
- Invalid config file (bad JSON/YAML syntax)
- Missing required config fields
- Invalid field values
- File permissions issue reading config

**Example:**
```
$ gitsync
[ERROR] Config error: invalid JSON in ~/.gitsync/config.json
$ echo $?
3
```

---

### Exit Code 4: Git Error

**Conditions:**
- Not inside a git repository
- Current branch doesn't exist
- Source remote URL mismatch
- Git command failed (unexpected)
- Branch filtering matched zero branches

**Example:**
```
$ gitsync
[ERROR] Not in a git repository
$ echo $?
4
```

---

### Exit Code 5: Permission/Access Error

**Conditions:**
- SSH key not found or invalid
- SSH key permission denied (403)
- SSH host key verification failed
- Config file not readable/writable
- Insufficient permissions for home directory

**Example:**
```
$ gitsync
[ERROR] SSH error: permission denied (public key)
$ echo $?
5
```

---

## Exception Mapping

**In C++ exception handling, map as follows:**

```cpp
try {
    // main() logic
} catch (const ConfigException& e) {
    spdlog::error("Config error: {}", e.what());
    return 3;  // Config error
} catch (const GitException& e) {
    spdlog::error("Git error: {}", e.what());
    return 4;  // Git error
} catch (const PermissionException& e) {
    spdlog::error("Permission error: {}", e.what());
    return 5;  // Permission error
} catch (const std::exception& e) {
    spdlog::error("Unexpected error: {}", e.what());
    return 1;  // General error
}
```

---

## Implementation Plan

### Phase 1: Define Exception Classes

**Files:**
- `include/aw/gitsync/config_reader.hpp`
- `src/config_reader.cpp`

**New exception types:**
```cpp
namespace aw::gitsync {

class GitSyncException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ConfigException : public GitSyncException {};
class GitException : public GitSyncException {};
class PermissionException : public GitSyncException {};

} // namespace aw::gitsync
```

### Phase 2: Review All Error Sites

**Files to scan:**
- `src/config_reader.cpp` — Config parsing → ConfigException
- `src/git_sync_service.cpp` — Git operations → GitException
- `src/process_runner.cpp` — Process execution → PermissionException or GitException
- `src/known_hosts_service.cpp` → PermissionException or GitException

**Changes:**
1. Identify where exceptions are thrown
2. Throw appropriate exception type (not just `std::runtime_error`)
3. Add meaningful error messages

### Phase 3: Handle SSH Key Errors

**File:**
- `src/process_runner.cpp`

**SSH-specific error detection:**
```cpp
if (stderr.find("permission denied") != std::string::npos) {
    throw PermissionException(stderr);
}
if (stderr.find("No such file or directory") != std::string::npos) {
    throw PermissionException("SSH key not found");
}
```

### Phase 4: Handle Git Errors

**File:**
- `src/git_sync_service.cpp`

**Git-specific error detection:**
```cpp
if (output.stderr.find("not a git repository") != std::string::npos) {
    throw GitException("Not in a git repository");
}
if (output.stderr.find("no matching commit") != std::string::npos) {
    throw GitException("Branch not found");
}
```

### Phase 5: Update Main Error Handler

**File:**
- `src/main.cpp`

**Changes:**
1. Add exception type checks
2. Map to exit codes
3. Example:

```cpp
int main(int argc, char* argv[]) {
    try {
        auto args = parse_args(argc, argv);
        auto config = ConfigReader::read_config();
        // ... rest of logic ...
        return 0;  // Success
    } catch (const aw::gitsync::ConfigException& e) {
        spdlog::error("Config error: {}", e.what());
        return 3;
    } catch (const aw::gitsync::GitException& e) {
        spdlog::error("Git error: {}", e.what());
        return 4;
    } catch (const aw::gitsync::PermissionException& e) {
        spdlog::error("Permission error: {}", e.what());
        return 5;
    } catch (const std::exception& e) {
        spdlog::error("Unexpected error: {}", e.what());
        return 1;
    }
}
```

### Phase 6: Handle Partial Success

**Files:**
- `src/git_sync_service.cpp`
- `src/main.cpp`

**Changes:**
1. After `run()`, check if any remotes failed
2. If some succeeded and some failed → exit code 2
3. If all failed → exit code 4 or 1 depending on error type

```cpp
auto results = git_sync_service->push_all(refspec);
auto [success, error, total] = git_sync_service->compute_summary(results);

if (error > 0) {
    if (success > 0) {
        return 2;  // Partial success
    } else {
        return 4;  // All failed (likely git/permission error)
    }
}
return 0;  // All succeeded
```

### Phase 7: Tests

**File:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Config error → exit code 3
2. Git error → exit code 4
3. Permission error → exit code 5
4. Partial success → exit code 2
5. All success → exit code 0
6. Unexpected error → exit code 1

**Test approach:**
```cpp
TEST_CASE("Exit codes") {
    REQUIRE(run_gitsync_with_bad_config() == 3);
    REQUIRE(run_gitsync_not_in_repo() == 4);
    REQUIRE(run_gitsync_with_bad_ssh_key() == 5);
    REQUIRE(run_gitsync_partial_failure() == 2);
    REQUIRE(run_gitsync_success() == 0);
}
```

---

## Acceptance Criteria

- ✅ All exit codes implemented and documented
- ✅ Exception types clearly map to exit codes
- ✅ All error sites throw appropriate exception
- ✅ Tests verify each exit code
- ✅ README updated with exit code reference
- ✅ CI can rely on exit codes for decision-making

---

## Documentation

**Update README.md:**

```markdown
## Exit Codes

gitsync uses exit codes to indicate the result of the sync operation:

| Code | Meaning | Action |
|------|---------|--------|
| 0 | Success | All remotes synced |
| 1 | General error | Unexpected failure |
| 2 | Partial success | Some remotes synced, some failed |
| 3 | Config error | Fix configuration and retry |
| 4 | Git error | Not in repo or git issue |
| 5 | Permission error | Fix SSH key or file permissions |

### Example: CI Integration

```bash
#!/bin/bash
gitsync
case $? in
  0) echo "All synced"; exit 0 ;;
  2) echo "Partial sync"; exit 0 ;;  # Don't fail CI
  *) echo "Sync failed"; exit 1 ;;
esac
```
```

---

## Example Usage in CI/CD

**GitHub Actions:**
```yaml
- name: Sync repos
  run: gitsync
  continue-on-error: true  # Don't fail on exit code 2

- name: Check sync result
  run: |
    gitsync
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ] || [ $EXIT_CODE -eq 2 ]; then
      echo "Sync OK"
    else
      echo "Sync failed with code $EXIT_CODE"
      exit 1
    fi
```

