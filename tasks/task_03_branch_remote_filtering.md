# Task 3: Branch & Remote Filtering

**Status:** Pending  
**Priority:** High  
**Complexity:** Medium  
**Sessions:** 1  
**Dependencies:** Task 1 (Config fields added)

---

## Objective

Implement branch and remote filtering logic. Users can now specify which branches/remotes to include/exclude, preventing accidental syncs of protected branches or specific remotes.

---

## Requirements

### 1. Branch Filtering

**Config fields (from Task 1):**
- `include_branches`: String array, default `["*"]` (include all)
- `exclude_branches`: String array, default `[]` (exclude none)

**Pattern matching:**
- Glob patterns: `*`, `?`, `[...]`
- Examples:
  - `"main"` — exact match
  - `"release/*"` — matches `release/1.0`, `release/2.0`, etc.
  - `"feature-*"` — matches `feature-new`, `feature-old`, etc.

**Logic:**
1. Get all branches from source remote
2. For each branch:
   - Include if matches any pattern in `include_branches`
   - **AND** does not match any pattern in `exclude_branches`
3. Only sync branches that pass the filter

**Default behavior:** With defaults, all branches are included (backward compatible).

### 2. Remote Filtering

**Config fields (from Task 1):**
- `include_remotes`: String array, default `["*"]` (all remotes)
- `exclude_remotes`: String array, default `["upstream", "base"]` (skip these)

**Pattern matching:**
- Exact names only (no wildcards needed; remotes are small set)
- If `include_remotes` contains `["*"]`, include all defined remotes
- Always respect `exclude_remotes` (removals have priority)

**Logic:**
1. Get all remotes from config
2. For each remote:
   - Include if name in `include_remotes` or `include_remotes == ["*"]`
   - **AND** name not in `exclude_remotes`
3. Only push to remotes that pass the filter

**Default behavior:** All remotes except `upstream` and `base` are synced.

### 3. Configuration Merge Behavior

**Problem:** When configs are merged across layers (global → org → repo), how should filter arrays combine?

**Solution:** Array merge (extend, don't replace)

```json
// Global: ~/.gitsync/config.json
{
  "exclude_branches": ["main"]
}

// Org: ../.gitsync/config.json
{
  "exclude_branches": ["develop"]
}

// Repo: ./.gitsync/config.json
{
  "exclude_branches": ["release/*"]
}

// Merged result
{
  "exclude_branches": ["main", "develop", "release/*"]
}
```

**Implementation:**
- In `ConfigSettings::merge()`, concatenate arrays instead of replacing
- Avoid duplicates (optional; depends on use case)

### 4. Logging & Diagnostics

**Log at DEBUG level:**
- List of branches that match filters (included)
- List of branches excluded (why)
- List of remotes that will be synced
- List of remotes excluded (why)

**Log at WARN level:**
- If filtering results in zero remotes (likely config error)
- If filtering results in zero branches (likely too restrictive)

---

## Implementation Plan

### Phase 1: Update Config Merge Logic

**Files:**
- `src/config_settings.cpp`
- `include/aw/gitsync/config_settings.hpp`

**Changes:**
1. Update `PartialConfigSettings::merge()` to concatenate filter arrays
2. Ensure no duplicates (optional; use `std::set` if desired)
3. Test with 3-layer merge

### Phase 2: Implement Branch Filtering

**Files:**
- `include/aw/gitsync/git_sync_service.hpp`
- `src/git_sync_service.cpp`

**New methods:**
```cpp
/// Check if branch should be synced based on include/exclude patterns
bool matches_branch_filter(const std::string& branch_name,
                          const std::vector<std::string>& include_patterns,
                          const std::vector<std::string>& exclude_patterns);

/// Get list of branches from source remote that pass filter
std::vector<std::string> get_filtered_branches();

/// Match glob pattern (e.g., "release/*" matches "release/1.0")
static bool glob_match(const std::string& pattern, const std::string& text);
```

**Logic:**
1. Run `git branch -r` to get all branches from source remote
2. For each branch, test against filters
3. Return list of branches to sync

### Phase 3: Implement Remote Filtering

**Files:**
- `include/aw/gitsync/git_sync_service.hpp`
- `src/git_sync_service.cpp`

**New methods:**
```cpp
/// Check if remote should be synced based on include/exclude lists
bool matches_remote_filter(const std::string& remote_name);

/// Get filtered list of remotes from config
std::vector<std::string> get_filtered_remotes();
```

**Logic:**
1. Get all remotes from `config_.remote_urls`
2. For each remote, check if in include/exclude lists
3. Return filtered list

### Phase 4: Integrate into Sync Flow

**Files:**
- `src/git_sync_service.cpp`

**Changes:**
1. In `run()`, call `get_filtered_branches()` and `get_filtered_remotes()`
2. Only build refspec for filtered branches
3. Only push to filtered remotes
4. Log filtered lists at DEBUG level

### Phase 5: Tests

**File:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Branch glob matching (exact, wildcard, complex patterns)
2. Remote exact matching
3. Config merge of filter arrays
4. Zero-match cases (warn)
5. Combined branch + remote filtering

---

## Acceptance Criteria

- ✅ Branch filtering with glob patterns works
- ✅ Remote filtering with exact names works
- ✅ Config merge concatenates filter arrays
- ✅ Zero-match cases are warned
- ✅ Filtered lists logged at DEBUG
- ✅ Backward compatible (defaults include all)
- ✅ All tests pass

---

## Examples

### Config: Exclude feature branches

```json
{
  "exclude_branches": ["feature/*", "wip/*"],
  "exclude_remotes": []
}
```

**Result:** Only sync branches like `main`, `develop`, `release/1.0` (not `feature/new-feature`)

### Config: Sync only to two remotes

```json
{
  "include_remotes": ["gitlab", "backup"],
  "exclude_remotes": []
}
```

**Result:** Even if config defines 5 remotes, only push to `gitlab` and `backup`

### Config: Per-org exclusions

```json
// Global (~/.gitsync/config.json)
{
  "exclude_branches": ["main"]
}

// Org (Company/.gitsync/config.json)
{
  "exclude_branches": ["develop"]
}
```

**Result:** Branches `main` and `develop` are both excluded

