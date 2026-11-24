# Task 1: Configuration Format Enhancements (YAML + snake_case)

**Status:** Pending  
**Priority:** High (foundation for other tasks)  
**Complexity:** High  
**Sessions:** 1  
**Dependencies:** None

---

## Objective

Add YAML config file support and migrate all JSON config fields from PascalCase to snake_case. This is a breaking change required for v1.0.0 stability.

---

## Supported Field Names (Dual Convention Support)

**Both PascalCase (v0.6.0) and snake_case (v1.0+) are supported:**

| Field | PascalCase (Existing) | snake_case (New) | Type | Default |
|---|---|---|---|---|
| Logging | `LogLevel` | `log_level` | string | `"info"` |
| Source Remote | `SourceRemoteName` | `source_remote_name` | string | `"origin"` |
| Branch Template | `RemoteBranchTemplate` | `remote_branch_template` | string | `"%owner%/%reponame%/%branchname%"` |
| Mirrors | `RemoteUrls` | `remote_urls` | object | `{}` |
| SSH Keys | `SshKeyTypes` | `ssh_key_types` | string | `""` |
| — | — | `max_retries` | integer | `1` |
| — | — | `include_branches` | array | `["*"]` |
| — | — | `exclude_branches` | array | `[]` |
| — | — | `include_remotes` | array | `["*"]` |
| — | — | `exclude_remotes` | array | `["upstream", "base"]` |

**Dual-convention parsing:** Existing configs with `LogLevel`, `RemoteUrls`, etc. continue to work. New configs can use snake_case. If both are present, snake_case takes precedence.

---

## Requirements

### 1. Configuration File Format Support

- **JSON:** Continue supporting `.json` files
- **YAML:** Add support for `.yaml` and `.yml` files
- **Auto-detection:** Determine format from file extension
- **Fallback:** If JSON parsing fails, attempt YAML parsing

### 2. Config Field Migration

- Rename all five existing fields to snake_case
- Add five new filter fields (see table above)
- Update all three config layers (global → org → repo)

### 3. Config Hierarchy Merge

- **Filter arrays** should merge across layers (combine patterns)
- **Other fields** should override (last layer wins)
- Example merge:
  ```json
  // Global
  { "exclude_branches": ["main"] }
  
  // Org
  { "exclude_branches": ["develop"] }
  
  // Repo
  { "exclude_branches": ["release/*"] }
  
  // Result
  { "exclude_branches": ["main", "develop", "release/*"] }
  ```

### 4. Backward Compatibility (Dual Convention)

- **Both PascalCase and snake_case fields are accepted**
- No deprecation warnings
- If both present, snake_case takes precedence
- Users can migrate at their own pace (zero pressure)

### 5. YAML Parsing Library

**Decision required:**
- Option A: Use `nlohmann/json` YAML integration (if available in v3.11.3)
- Option B: Add `yaml-cpp` library via FetchContent
- Option C: Minimal built-in YAML parser for basic configs

**Recommendation:** Option B (yaml-cpp) for robustness and correctness.

---

## Implementation Plan

### Phase 1: Update Data Structures

**Files:**
- `include/aw/gitsync/config_settings.hpp`
- `src/config_settings.cpp`

**Changes:**
1. Rename fields in `ConfigSettings` struct
2. Rename fields in `PartialConfigSettings` struct
3. Update `merge()` logic to handle array merging for filters
4. Update `into_final_settings()` with new defaults

### Phase 2: Update Config Reader

**Files:**
- `src/config_reader.cpp`
- `include/aw/gitsync/config_reader.hpp`

**Changes:**
1. Add YAML library to dependencies (CMake)
2. Implement file format detection (by extension)
3. Add YAML parsing function
4. Add JSON parsing function (update to snake_case)
5. Add deprecation warning for PascalCase fields
6. Update config hierarchy logic (3-layer merge)

### Phase 3: Update CMakeLists.txt

**File:**
- `CMakeLists.txt`

**Changes:**
```cmake
FetchContent_Declare(
    yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS NAMES yaml-cpp
)
# Add to FetchContent_MakeAvailable()
```

Also update `gitsync_core` target to link `yaml-cpp`.

### Phase 4: Update Documentation

**Files:**
- `docs/README.md`

**Changes:**
1. Add YAML examples (Scenario with YAML config)
2. Add snake_case field reference
3. Add migration guide (old → new field names)
4. Document filter fields (branch/remote inclusion/exclusion)

### Phase 5: Update Tests

**Files:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Load JSON config with snake_case fields
2. Load YAML config with same fields
3. Verify config hierarchy merge (3 layers)
4. Verify array merge for filters
5. Verify defaults are applied
6. Load JSON with old PascalCase (verify deprecation warning)

---

## Acceptance Criteria

- ✅ All config fields renamed to snake_case across codebase
- ✅ YAML files load and parse correctly
- ✅ Config hierarchy works for both JSON and YAML
- ✅ Filter fields stored in `ConfigSettings` (not yet applied to sync logic)
- ✅ Deprecation warning logged for old PascalCase configs
- ✅ Both PascalCase (old) and snake_case (new) fields accepted
- ✅ YAML files load and parse correctly
- ✅ Config hierarchy works for both JSON and YAML
- ✅ New filter fields stored (not yet applied to sync logic)
- ✅ Dual-convention parsing (snake_case takes precedence if both present)
- ✅ No breaking changes to existing configs
- ✅ All existing tests pass
- ✅ New tests added for JSON/YAML loading with both conventions
- ✅ Config docs updated with examples (both conventions)
- ✅ CMake builds without errors

---

## Example Configurations

### JSON (v0.6.0 format — still works)

```json
{
  "LogLevel": "info",
  "SourceRemoteName": "origin",
  "RemoteBranchTemplate": "%owner%/%reponame%/%branchname%",
  "RemoteUrls": {
    "gitlab": "git@gitlab.com:user/backup.git",
    "github": "git@github.com:user/mirror.git"
  },
  "SshKeyTypes": "ed25519,rsa"
}
```

### JSON (v1.0 format — new convention, optional)

```json
{
  "log_level": "info",
  "source_remote_name": "origin",
  "remote_branch_template": "%owner%/%reponame%/%branchname%",
  "max_retries": 1,
  "ssh_key_types": "ed25519,rsa",
  "remote_urls": {
    "gitlab": "git@gitlab.com:user/backup.git",
    "github": "git@github.com:user/mirror.git"
  },
  "exclude_branches": ["main", "master"],
  "exclude_remotes": ["upstream", "base"]
}
```

### YAML (v1.0 format — new support, can use either convention)

```yaml
log_level: debug
source_remote_name: origin
remote_branch_template: "%owner%/%reponame%/%branchname%"
max_retries: 2
ssh_key_types: "ed25519,rsa"

remote_urls:
  gitlab: "git@gitlab.com:user/backup.git"
  github: "git@github.com:user/mirror.git"

exclude_branches:
  - main
  - master
  - release/*

exclude_remotes:
  - upstream
  - base
```

---

## Notes

- This task is a foundation. Subsequent tasks (filtering, retries) depend on these new config fields being present.
- The merge behavior for filter arrays is critical to get right; test thoroughly.
- YAML parsing may add ~20-30 seconds to initial build (yaml-cpp is small but compile-heavy). Consider it acceptable for the feature.

