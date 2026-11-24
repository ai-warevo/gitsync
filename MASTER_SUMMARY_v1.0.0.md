# gitsync v1.0.0 Release Master Summary

**Status:** Planned  
**Release Target:** v1.0.0 (Breaking Changes Release)  
**Total Sessions:** ~7  
**Total Tasks:** 7

---

## Overview

This document summarizes all tasks for the gitsync v1.0.0 breaking-change release. Each task is designed to be completed in roughly one session, with clear dependencies and acceptance criteria.

**Key themes:**
1. **Configuration system overhaul** (snake_case, YAML, filtering)
2. **CLI enhancements** (dry-run, verbosity, JSON output)
3. **Reliability** (retry logic, parallel pushes)
4. **Professional quality** (exit codes, permissions check, public API stability)

---

## Task Breakdown

### Task 1: Configuration Format Enhancements (YAML + snake_case)
**Priority:** 🔴 Critical  
**Complexity:** 🔴 High  
**Sessions:** 1  
**Status:** Pending  
**Dependencies:** None

**What:** Migrate config fields from PascalCase to snake_case; add YAML support and new filter fields.

**Deliverables:**
- PascalCase → snake_case migration (all 5 existing fields)
- YAML file support (`.yaml`, `.yml` extensions)
- New config fields: `max_retries`, `include_branches`, `exclude_branches`, `include_remotes`, `exclude_remotes`
- Config hierarchy merge for filter arrays
- Deprecation warning for old PascalCase configs

**Files:** See `tasks/task_01_config_yaml_snake_case.md`

---

### Task 2: Command-Line Interface Enhancements
**Priority:** 🟢 High  
**Complexity:** 🟡 Medium  
**Sessions:** 1  
**Status:** Pending  
**Dependencies:** Task 1

**What:** Add CLI flags for dry-run, verbosity, and JSON output.

**Deliverables:**
- `-d, --dry-run` flag (preview without pushing)
- `-vv, --verbose` flag (DEBUG logging)
- `-q, --quiet` flag (ERROR logging)
- `--json` flag (structured output for CI)
- Improved help message

**Files:** See `tasks/task_02_cli_flags.md`

---

### Task 3: Branch & Remote Filtering
**Priority:** 🟢 High  
**Complexity:** 🟡 Medium  
**Sessions:** 1  
**Status:** Pending  
**Dependencies:** Task 1 (config fields added)

**What:** Implement branch/remote inclusion/exclusion logic.

**Deliverables:**
- Branch glob pattern matching (include/exclude)
- Remote exact name matching (include/exclude)
- Config merge for filter arrays
- Logging of filtered results
- Zero-match warnings

**Files:** See `tasks/task_03_branch_remote_filtering.md`

---

### Task 4a: Parallel Remote Pushes
**Priority:** 🟡 Medium  
**Complexity:** 🔴 High  
**Sessions:** 1  
**Status:** Pending  
**Dependencies:** Task 2 (CLI framework)

**What:** Push to multiple remotes concurrently with `--jobs=N` flag.

**Deliverables:**
- `--jobs=N` CLI flag (default 1, sequential)
- Thread pool or async implementation
- Thread-safe result aggregation
- Performance improvement with parallelism

**Files:** See `tasks/task_04a_parallel_pushes.md`

---

### Task 4b: Retry Logic
**Priority:** 🟢 High  
**Complexity:** 🟡 Medium  
**Sessions:** 1  
**Status:** Pending  
**Dependencies:** Task 1 (config field `max_retries`)

**What:** Automatic retry with exponential backoff for failed pushes.

**Deliverables:**
- `max_retries` config applied (default: 1)
- Exponential backoff: 2^(attempt-1) seconds
- Clear logging of retry attempts
- Graceful failure after retries exhausted

**Files:** See `tasks/task_04b_retry_logic.md`

---

### Task 5: Exit Codes Standardization
**Priority:** 🟢 High  
**Complexity:** 🟢 Low  
**Sessions:** 0.5  
**Status:** Pending  
**Dependencies:** Tasks 1–4 (all features complete)

**What:** Define standard exit codes for different failure modes.

**Deliverables:**
- Exit code 0: Success
- Exit code 1: General error
- Exit code 2: Partial success
- Exit code 3: Config error
- Exit code 4: Git error
- Exit code 5: Permission error

**Files:** See `tasks/task_05_exit_codes.md`

---

### Task 6: JSON Output & Permission Check (Combined)
**Priority:** 🟡 Medium  
**Complexity:** 🟢 Low  
**Sessions:** 0.5  
**Status:** Pending  
**Dependencies:** Task 2 (JSON output format defined), Tasks 1–5 (all features)

**What:** Full JSON output for CI; warn if config is world-readable.

**Deliverables:**
- Complete JSON schema with all result details
- Serialization of sync results to JSON
- Permission check: warn if `.gitsync/config.json` is world-readable
- Clear security suggestion

**Files:** See `tasks/task_06_json_output_permissions.md`

---

### Task 7: Stabilize Public API & Header Exports
**Priority:** 🟡 Medium  
**Complexity:** 🟡 Medium  
**Sessions:** 1  
**Status:** Pending  
**Dependencies:** All prior tasks (complete feature set)

**What:** Clarify stable vs internal APIs; enable library usage.

**Deliverables:**
- Public API header (`include/aw/gitsync/public/gitsync.hpp`)
- Library usage documentation (`docs/LIBRARY_USAGE.md`)
- CMake export configuration (`find_package(gitsync)`)
- Install targets added
- Internal headers marked "INTERNAL"

**Files:** See `tasks/task_07_public_api_stabilization.md`

---

## Implementation Schedule

**Recommended order:**

| Week | Session | Task | Complexity | Depends On |
|------|---------|------|-----------|-----------|
| 1 | 1 | Task 1 (Config) | High | — |
| 1 | 2 | Task 2 (CLI) | Medium | Task 1 |
| 1 | 3 | Task 3 (Filtering) | Medium | Task 1 |
| 2 | 4 | Task 4a (Parallel) | High | Task 2 |
| 2 | 5 | Task 4b (Retry) | Medium | Task 1 |
| 2 | 6 | Task 5 (Exit Codes) | Low | Tasks 1–4 |
| 2 | 7 | Task 6 (JSON/Perms) | Low | Tasks 1–5 |
| 3 | 8 | Task 7 (Public API) | Medium | All |

---

## Breaking Changes (v0.6.0 → v1.0.0)

**Users upgrading from v0.6.0:**

1. **No required changes to existing configs**
   - Both PascalCase and snake_case field names are supported
   - Existing configs continue to work as-is
   
2. **Exit codes changed (CI/CD scripts may need updates):**
   - 0 = success (unchanged)
   - 2 = partial success (new)
   - 3 = config error (new)
   - 4 = git error (new)
   - 5 = permission error (new)
   - Other = general error

3. **New default behavior:**
   - `exclude_remotes` defaults to `["upstream", "base"]` (may prevent some sync operations)

**Optional improvements:**
   - New configs can use snake_case convention (more idiomatic C++)
   - Add new filter fields to config to control branch/remote sync

---

## Migration Guide (v0.6.0 → v1.0.0)

**For users:**

1. No changes required to existing configs — they still work
2. Exit codes may differ — update CI/CD scripts if needed (see Task 5 for mapping)
3. Test new features with `--dry-run` to preview changes
4. Optionally migrate config to snake_case or add new filter fields

**For library consumers:**

1. Update include path: `#include <aw/gitsync/public/gitsync.hpp>`
2. Update CMake: `find_package(gitsync 1.0 REQUIRED)` and `target_link_libraries(app gitsync::public)`
3. Catch `aw::gitsync::{Config,Git,Permission}Exception` for specific error handling

**Example: Optional config upgrade (not required)**

```json
// Before (v0.6.0) — still works in v1.0.0
{
  "LogLevel": "info",
  "RemoteUrls": {
    "gitlab": "git@gitlab.com:user/backup.git"
  }
}

// After (v1.0.0, optional) — cleaner snake_case
{
  "log_level": "info",
  "remote_urls": {
    "gitlab": "git@gitlab.com:user/backup.git"
  },
  "exclude_remotes": ["upstream", "base"],
  "max_retries": 1
}
```

---

## Feature Checklist

**Core Features (Existing):**
- ✅ Multi-layer config hierarchy (global → org → repo)
- ✅ SSH known_hosts management
- ✅ Template-based branch naming
- ✅ Cross-platform (Linux & Windows)

**New Features (v1.0.0):**
- 🔄 Configuration: YAML + snake_case
- 🔄 CLI: dry-run, verbosity, JSON output
- 🔄 Filtering: branch/remote inclusion/exclusion
- 🔄 Reliability: parallel pushes, retry logic
- 🔄 Quality: exit codes, permission warnings
- 🔄 API: Stable public headers, library usage guide

---

## Testing Strategy

**Each task includes:**
- Unit tests (mocked dependencies)
- Integration tests (real git operations, local only)
- Error cases (config errors, git failures, permission issues)

**Combined test suite:**
- All features tested together
- Exit codes verified for all scenarios
- JSON output validated for CI integration
- Permission checks verified on all platforms

---

## Documentation Updates

**Files to update/create:**

| File | Type | Status |
|------|------|--------|
| `README.md` | Update | Add Library section, exit codes, new flags |
| `docs/README.md` (config) | Update | Add YAML examples, snake_case fields, new filter fields |
| `docs/LIBRARY_USAGE.md` | Create | Library integration guide, examples, CMake setup |
| `BREAKING_CHANGES_v1.0.0.md` | Create | Breaking changes summary, migration path |
| `PUBLIC_API_STABILIZATION.md` | Create | Public/internal API clarification |
| Various `tasks/task_*.md` | Create | Detailed task specifications |

---

## Success Criteria

**v1.0.0 is complete when:**

- ✅ All 7 tasks implemented and tested
- ✅ All breaking changes documented and migrated
- ✅ Exit codes standardized and working
- ✅ JSON output validated for CI use
- ✅ Public API stable and documented
- ✅ Zero breaking changes after v1.0.0 within v1.x
- ✅ CHANGELOG.md updated (auto-generated by CI)
- ✅ GitHub Release published with migration guide

---

## Questions & Decisions

**To confirm before starting:**

1. ✅ Namespace: Keep `aw::gitsync` or rename to just `gitsync`?
   - **Decision:** Keep `aw::gitsync` (consistent with ai-warevo projects)

2. ✅ YAML library choice: `yaml-cpp` or nlohmann/json YAML integration?
   - **Decision:** `yaml-cpp` for robustness (config is critical)

3. ✅ Parallel implementation: `std::async` or thread pool library?
   - **Decision:** `std::async` (no new dependencies, proven)

4. ✅ Release strategy: v1.0.0-rc1 or direct to v1.0.0?
   - **Decision:** Recommend RC1 for user testing before final release

---

## References

- Main plan: `BREAKING_CHANGES_v1.0.0.md`
- API design: `PUBLIC_API_STABILIZATION.md`
- Individual tasks: `tasks/task_*.md`
- Current README: `README.md`
- Current config docs: `docs/README.md`

---

## Next Steps

1. Review this master summary
2. Confirm decisions above
3. Start Task 1 (config enhancements)
4. Follow recommended schedule
5. Update docs as features are completed
6. Test thoroughly before RC1
7. Gather user feedback
8. Final release v1.0.0

