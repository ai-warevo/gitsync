# Task 7: Stabilize Public API & Header Exports

**Status:** Pending  
**Priority:** Medium  
**Complexity:** Medium  
**Sessions:** 1  
**Dependencies:** All previous tasks (complete feature set for 1.0)

---

## Objective

Clarify which APIs are stable for library consumers vs internal implementation details. Establish a contract that allows the gitsync library to be safely used by other C++ projects without breaking on patch/minor updates.

---

## Background

See `PUBLIC_API_STABILIZATION.md` for detailed analysis. This task implements that design.

---

## Public API Surface (Stable v1.0+)

**These are guaranteed stable within v1.x:**

```cpp
namespace aw::gitsync {

// Configuration struct (data container, stable ABI)
struct ConfigSettings { /* ... */ };

// Config reader (entry point for loading)
class ConfigReader {
public:
    static ConfigSettings read_config();  // May throw ConfigException
};

// Process runner interface (dependency injection)
class IProcessRunner {
public:
    virtual ~IProcessRunner() = default;
    struct Result { bool success; std::string stdout; std::string stderr; };
    virtual Result run(const std::string& cmd, 
                      const std::vector<std::string>& args) = 0;
};

// Concrete process runner
class ProcessRunner : public IProcessRunner {
public:
    Result run(const std::string& cmd, 
              const std::vector<std::string>& args) override;
};

// Main service (core functionality)
class GitSyncService {
public:
    GitSyncService(std::shared_ptr<IProcessRunner> runner,
                  const ConfigSettings& config);
    
    void run();  // May throw GitException or PermissionException
    
    std::vector<std::pair<std::string, bool>>
    push_all(const std::string& refspec);
    
    std::tuple<uint32_t, uint32_t, uint32_t>
    compute_summary(const std::vector<std::pair<std::string, bool>>& results);
    
    // Static helpers (for testing, not instance-dependent)
    static std::string build_remote_branch(const std::string& template_str,
                                          const std::string& origin_url,
                                          const std::string& branch_name);
    static std::string sanitize_branch_name(const std::string& name);
};

} // namespace aw::gitsync
```

---

## Internal APIs (Use at Own Risk)

**These are not part of the stability contract:**

- `KnownHostsService` (implementation detail of `run()`)
- `utils.hpp` helpers (internal utilities)
- Private methods on public classes
- Anything in `src/` not exposed in public headers

---

## Implementation Plan

### Phase 1: Mark Headers with Documentation

**Files to update:**
- `include/aw/gitsync/config_settings.hpp`
- `include/aw/gitsync/config_reader.hpp`
- `include/aw/gitsync/git_sync_service.hpp`
- `include/aw/gitsync/process_runner.hpp`
- `include/aw/gitsync/known_hosts_service.hpp` (mark as INTERNAL)
- `include/aw/gitsync/utils.hpp` (mark as INTERNAL)

**Example update (config_settings.hpp):**
```cpp
#ifndef AW_GITSYNC_CONFIG_SETTINGS_HPP
#define AW_GITSYNC_CONFIG_SETTINGS_HPP

// STABLE PUBLIC API (v1.0+)
// This header is part of the gitsync C++ library ABI.
// All members, types, and function signatures are stable within v1.x.
// Breaking changes only in v2.0+.
//
// See: docs/LIBRARY_USAGE.md for integration guide.

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace aw::gitsync {

/// User-facing configuration struct.
/// STABLE: Structure will not change within v1.x.
struct ConfigSettings {
    // ... members ...
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_CONFIG_SETTINGS_HPP
```

**Example update (known_hosts_service.hpp):**
```cpp
#ifndef AW_GITSYNC_KNOWN_HOSTS_SERVICE_HPP
#define AW_GITSYNC_KNOWN_HOSTS_SERVICE_HPP

// INTERNAL IMPLEMENTATION DETAIL
// This header is NOT part of the public API.
// Do not depend on this in external code.
// This class may change or disappear without notice.

namespace aw::gitsync {

class KnownHostsService { /* ... */ };

} // namespace aw::gitsync

#endif
```

### Phase 2: Create Public API Header

**New file: `include/aw/gitsync/public/gitsync.hpp`**

```cpp
#ifndef AW_GITSYNC_PUBLIC_GITSYNC_HPP
#define AW_GITSYNC_PUBLIC_GITSYNC_HPP

// GitSync v1.0 Public C++ API
// Stable, versioned API for external library consumers.
// Recommended entry point: include this header, not individual internals.

#include "../config_settings.hpp"
#include "../config_reader.hpp"
#include "../git_sync_service.hpp"
#include "../process_runner.hpp"

#define GITSYNC_API_VERSION "1.0.0"
#define GITSYNC_API_VERSION_MAJOR 1
#define GITSYNC_API_VERSION_MINOR 0
#define GITSYNC_API_VERSION_PATCH 0

namespace aw::gitsync {

// Exported symbols (stable for v1.x):
//   - ConfigSettings struct
//   - ConfigReader::read_config()
//   - GitSyncService class
//   - IProcessRunner interface
//   - ProcessRunner implementation
//
// All other classes/headers are internal and may change.
//
// Usage:
//   #include <aw/gitsync/public/gitsync.hpp>
//   auto config = aw::gitsync::ConfigReader::read_config();
//   auto runner = std::make_shared<aw::gitsync::ProcessRunner>();
//   aw::gitsync::GitSyncService sync(runner, config);
//   sync.run();

} // namespace aw::gitsync

#endif // AW_GITSYNC_PUBLIC_GITSYNC_HPP
```

### Phase 3: Create Documentation

**New file: `docs/LIBRARY_USAGE.md`**

```markdown
# Using gitsync as a C++ Library

If you want to embed gitsync synchronization in your own C++ application, you can link against `libgitsync_core`.

## Public API (Stable v1.0+)

The v1.0 stable public API consists of:

- `aw::gitsync::ConfigSettings` — Configuration struct
- `aw::gitsync::ConfigReader::read_config()` — Load config from files
- `aw::gitsync::GitSyncService` — Main sync engine
- `aw::gitsync::IProcessRunner` — Process execution interface
- `aw::gitsync::ProcessRunner` — Default process runner

**Include header:**
```cpp
#include <aw/gitsync/public/gitsync.hpp>
```

Do not include internal headers directly (e.g., `known_hosts_service.hpp`).

## Basic Example

```cpp
#include <aw/gitsync/public/gitsync.hpp>
#include <memory>
#include <iostream>

int main() {
    try {
        // Load configuration from ~/.gitsync/config.json and local overrides
        auto config = aw::gitsync::ConfigReader::read_config();
        
        // Create a process runner (or mock one for testing)
        auto runner = std::make_shared<aw::gitsync::ProcessRunner>();
        
        // Create and run the sync service
        aw::gitsync::GitSyncService sync(runner, config);
        sync.run();
        
        std::cout << "Sync completed successfully\n";
        return 0;
    } catch (const aw::gitsync::ConfigException& e) {
        std::cerr << "Config error: " << e.what() << "\n";
        return 3;
    } catch (const aw::gitsync::GitException& e) {
        std::cerr << "Git error: " << e.what() << "\n";
        return 4;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
```

## Dependency Injection Example

You can provide your own `IProcessRunner` implementation for testing or custom behavior:

```cpp
class MockProcessRunner : public aw::gitsync::IProcessRunner {
public:
    Result run(const std::string& cmd, 
              const std::vector<std::string>& args) override {
        // Return mock results instead of running real processes
        return {true, "mock output", ""};
    }
};

int main() {
    auto config = aw::gitsync::ConfigReader::read_config();
    auto mock_runner = std::make_shared<MockProcessRunner>();
    
    aw::gitsync::GitSyncService sync(mock_runner, config);
    sync.run();  // Uses mock runner instead of real processes
}
```

## Exception Handling

The public API throws exception types in `aw::gitsync` namespace:

- `ConfigException` — Configuration errors
- `GitException` — Git operation failures
- `PermissionException` — SSH/file permission issues

Catch these by type for appropriate error handling.

## CMake Integration

In your `CMakeLists.txt`:

```cmake
find_package(gitsync 1.0 REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app gitsync::public)
```

## API Stability Contract

**Stability within v1.x (patch/minor updates):**
- Public API (declared in `public/gitsync.hpp`) is stable
- Struct member types won't change
- Function signatures won't change
- New optional features may be added without breaking changes

**Potential breaking changes in v2.0:**
- Public API may be redesigned
- Struct layouts may change
- Function signatures may change
- Migration guide will be provided

## Internal APIs (Not Stable)

Do NOT depend on:
- `KnownHostsService`
- Anything in `include/aw/gitsync/utils.hpp`
- Private methods on public classes
- Anything marked `// INTERNAL`

These may change in any update without notice.

## Versioning

Check the API version at runtime:

```cpp
#include <iostream>
#include <aw/gitsync/public/gitsync.hpp>

std::cout << "GitSync API Version: " << GITSYNC_API_VERSION << "\n";
// Output: GitSync API Version: 1.0.0
```

## Questions?

Refer to `docs/README.md` for CLI usage and configuration guide.
```

### Phase 4: Update CMakeLists.txt for Export

**File: `CMakeLists.txt`**

Add after library creation:

```cmake
# Export public headers for find_package()
target_sources(gitsync_core PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/aw/gitsync/config_settings.hpp
        include/aw/gitsync/config_reader.hpp
        include/aw/gitsync/git_sync_service.hpp
        include/aw/gitsync/process_runner.hpp
        include/aw/gitsync/public/gitsync.hpp
)

# CMake package export
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gitsyncConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gitsyncConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gitsyncConfig.cmake"
    INSTALL_DESTINATION lib/cmake/gitsync
)

install(TARGETS gitsync_core EXPORT gitsyncTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    FILE_SET HEADERS
)

install(EXPORT gitsyncTargets
    FILE gitsyncTargets.cmake
    NAMESPACE gitsync::
    DESTINATION lib/cmake/gitsync
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gitsyncConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/gitsyncConfigVersion.cmake"
    DESTINATION lib/cmake/gitsync
)
```

### Phase 5: Create CMake Config Template

**New file: `cmake/gitsyncConfig.cmake.in`**

```cmake
@PACKAGE_INIT@

include("${CMAKE_CURRENT_DIR}/gitsyncTargets.cmake")

# Convenience target for public API
if(NOT TARGET gitsync::public)
    add_library(gitsync::public INTERFACE IMPORTED)
    target_link_libraries(gitsync::public INTERFACE gitsync::gitsync_core)
endif()

check_required_components(gitsync)
```

### Phase 6: Update README.md

**Add section to `README.md`:**

```markdown
## Using gitsync as a Library

The `gitsync` package can be used as a C++ library in your own projects.

**Stable v1.0 public API:**
- `aw::gitsync::ConfigSettings`
- `aw::gitsync::ConfigReader`
- `aw::gitsync::GitSyncService`
- `aw::gitsync::IProcessRunner`
- `aw::gitsync::ProcessRunner`

See [docs/LIBRARY_USAGE.md](docs/LIBRARY_USAGE.md) for detailed examples and integration guide.

```

### Phase 7: Tests

**Files:**
- `tests/gitsync_tests.cpp`

**Add tests:**
1. Public headers compile independently
2. CMake `find_package()` works
3. CMake targets are correct
4. Can link and use library

**Test setup:**
```cpp
// Test that public header is self-contained
#include <aw/gitsync/public/gitsync.hpp>

TEST_CASE("Public API version") {
    REQUIRE(GITSYNC_API_VERSION_MAJOR == 1);
    REQUIRE(GITSYNC_API_VERSION_MINOR == 0);
}

TEST_CASE("Public classes instantiate") {
    auto runner = std::make_shared<aw::gitsync::ProcessRunner>();
    REQUIRE(runner != nullptr);
}
```

---

## Acceptance Criteria

- ✅ Public API clearly marked (headers, documentation)
- ✅ Internal APIs marked as "INTERNAL"
- ✅ Public API header created (`public/gitsync.hpp`)
- ✅ Library usage guide created (`docs/LIBRARY_USAGE.md`)
- ✅ CMake export configured for `find_package()`
- ✅ Install targets added to CMakeLists.txt
- ✅ README updated with library section
- ✅ Tests verify public API works
- ✅ No breaking changes to public symbols

---

## Impact

**Before:** No clear signal about API stability; users unsure what's safe to depend on.

**After:** Clear contract; library users know public APIs won't break in v1.x; can confidently integrate gitsync into their projects.

---

## Future Versions

**v1.1, v1.2, etc.:** Public API unchanged, internal APIs may evolve freely

**v2.0:** Major version bump; may introduce breaking changes to public API (with migration guide)

