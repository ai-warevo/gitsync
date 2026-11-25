# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.9] - 2026-11-25

## [2.0.8] - 2026-11-25

### Changed

- WIP

## [2.0.7] - 2026-11-25

## [2.0.6] - 2026-11-24

## [2.0.5] - 2026-11-24

## [2.0.4] - 2026-11-24

### Changed

- Merge branch 'main' of github.com:ai-warevo/gitsync
- ci

## [2.0.3] - 2026-11-24

### Changed

- ci
- ci

## [2.0.2] - 2026-11-24

### Changed

- fix release
- fix
- upd release

### Fixed

- -u updater

## [2.0.1] - 2026-11-24

### Changed

- Update README.md

## [2.0.0] - 2026-11-24

### Breaking

- fix BREAKING CHANGES

### Added

- fix BREAKING CHANGES
- Add parallel pushes, retry logic, and JSON output
- Add CLI flags, branch/remote filtering, and exit codes
- Stabilize public API for library consumers v1.0+
- Add YAML support and dual PascalCase/snake_case with separation of concerns
- add certificate generation script for Windows code signing
- add Windows binary metadata (version info, icon, code signing)
- implement update check feature (-u flag)
- add multi-platform binary builds to release workflow
- rewrite gitsync from Rust to C++23

### Changed

- Merge branch 'main' of github.com:ai-warevo/gitsync
- fix build
- version 1.0.0
- fix
- fixes
- Merge branch 'main' of github.com:ai-warevo/gitsync
- update release quick-start with Windows setup and config hierarchy
- rename release binaries to gitsync/gitsync.exe
- version 0.6.0
- fix cert
- add certificate import from GitHub secrets for Windows code signing
- remove GITSYNC_CODE_SIGN option - now gracefully handles missing cert
- disable code signing - certificate not installed
- add logging to FetchContent and CMAKE_MODULE_PATH for Windows build diagnostics
- add Windows build instructions with cache clearing step
- adopt comprehensive preset system from spoofiq
- Merge branch 'main' of github.com:ai-warevo/gitsync
- update CHANGELOG for Windows metadata features
- version 0.5.0
- ci :3
- version 0.4.0
- use reusable-release.yml like tpl
- Merge branch 'main' of github.com:ai-warevo/gitsync
- slop
- update CHANGELOG for v0.3.0 release
- version 0.3.0
- version 0.2.0
- fix
- use aw_add_library instead of aw_core_library
- replace vendored dependencies with FetchContent
- update to new workflow structure
- add .clang-tidy and .editorconfig config files
- remove unnecessary documentation files
- add final project completion summary
- use C++23 instead of Rust

### Fixed

- ci
- ai
- resolve MSVC C4267 and C4456 warnings
- move certificate import condition inside powershell script
- correct secrets condition syntax in release workflow
- add GITSYNC_CODE_SIGN option, disable in CI
- add Windows popen/pclose compatibility defines
- disable GITSYNC_BUILD_TESTS in release-msvc preset
- update template_cpp to 123e346 with Windows/WindowsSDK fix
- pin template_cpp to working commit c25898a with CodeSign fixes + re-enable code signing
- force FetchContent to update aw_cmake_modules from main
- make clean also removes _deps cache to force FetchContent update
- use template_cpp commit c25898a with correct WindowsSDK include + add icon
- remove GIT_SHALLOW to ensure fresh fetch of template_cpp main branch
- correct aw_code_sign_target() TARGET parameter syntax
- use Ninja Multi-Config for MSVC presets (more portable than VS generator)
- correct aw_win_version() OUTVAR parameter syntax
- Windows CMake presets and build-release target
- override changelog-release defaults (remove boilerplate)
- Windows compatibility - use _popen/_pclose and safe getenv
- use ilammy/msvc-dev-cmd and cmake-build action for Windows

## [1.0.0] - 2026-11-24

### Breaking

- fix BREAKING CHANGES

### Added

- fix BREAKING CHANGES
- Add parallel pushes, retry logic, and JSON output
- Add CLI flags, branch/remote filtering, and exit codes
- Stabilize public API for library consumers v1.0+
- Add YAML support and dual PascalCase/snake_case with separation of concerns
- add certificate generation script for Windows code signing
- add Windows binary metadata (version info, icon, code signing)
- implement update check feature (-u flag)
- add multi-platform binary builds to release workflow
- rewrite gitsync from Rust to C++23

### Changed

- fix
- fixes
- Merge branch 'main' of github.com:ai-warevo/gitsync
- update release quick-start with Windows setup and config hierarchy
- rename release binaries to gitsync/gitsync.exe
- version 0.6.0
- fix cert
- add certificate import from GitHub secrets for Windows code signing
- remove GITSYNC_CODE_SIGN option - now gracefully handles missing cert
- disable code signing - certificate not installed
- add logging to FetchContent and CMAKE_MODULE_PATH for Windows build diagnostics
- add Windows build instructions with cache clearing step
- adopt comprehensive preset system from spoofiq
- Merge branch 'main' of github.com:ai-warevo/gitsync
- update CHANGELOG for Windows metadata features
- version 0.5.0
- ci :3
- version 0.4.0
- use reusable-release.yml like tpl
- Merge branch 'main' of github.com:ai-warevo/gitsync
- slop
- update CHANGELOG for v0.3.0 release
- version 0.3.0
- version 0.2.0
- fix
- use aw_add_library instead of aw_core_library
- replace vendored dependencies with FetchContent
- update to new workflow structure
- add .clang-tidy and .editorconfig config files
- remove unnecessary documentation files
- add final project completion summary
- use C++23 instead of Rust

### Fixed

- ci
- ai
- resolve MSVC C4267 and C4456 warnings
- move certificate import condition inside powershell script
- correct secrets condition syntax in release workflow
- add GITSYNC_CODE_SIGN option, disable in CI
- add Windows popen/pclose compatibility defines
- disable GITSYNC_BUILD_TESTS in release-msvc preset
- update template_cpp to 123e346 with Windows/WindowsSDK fix
- pin template_cpp to working commit c25898a with CodeSign fixes + re-enable code signing
- force FetchContent to update aw_cmake_modules from main
- make clean also removes _deps cache to force FetchContent update
- use template_cpp commit c25898a with correct WindowsSDK include + add icon
- remove GIT_SHALLOW to ensure fresh fetch of template_cpp main branch
- correct aw_code_sign_target() TARGET parameter syntax
- use Ninja Multi-Config for MSVC presets (more portable than VS generator)
- correct aw_win_version() OUTVAR parameter syntax
- Windows CMake presets and build-release target
- override changelog-release defaults (remove boilerplate)
- Windows compatibility - use _popen/_pclose and safe getenv
- use ilammy/msvc-dev-cmd and cmake-build action for Windows

## [0.6.0] - 2026-11-24

### Added

- add certificate generation script for Windows code signing
- add Windows binary metadata (version info, icon, code signing)
- implement update check feature (-u flag)
- add multi-platform binary builds to release workflow
- rewrite gitsync from Rust to C++23

### Changed

- fix cert
- add certificate import from GitHub secrets for Windows code signing
- remove GITSYNC_CODE_SIGN option - now gracefully handles missing cert
- disable code signing - certificate not installed
- add logging to FetchContent and CMAKE_MODULE_PATH for Windows build diagnostics
- add Windows build instructions with cache clearing step
- adopt comprehensive preset system from spoofiq
- Merge branch 'main' of github.com:ai-warevo/gitsync
- update CHANGELOG for Windows metadata features
- version 0.5.0
- ci :3
- version 0.4.0
- use reusable-release.yml like tpl
- Merge branch 'main' of github.com:ai-warevo/gitsync
- slop
- update CHANGELOG for v0.3.0 release
- version 0.3.0
- version 0.2.0
- fix
- use aw_add_library instead of aw_core_library
- replace vendored dependencies with FetchContent
- update to new workflow structure
- add .clang-tidy and .editorconfig config files
- remove unnecessary documentation files
- add final project completion summary
- use C++23 instead of Rust

### Fixed

- move certificate import condition inside powershell script
- correct secrets condition syntax in release workflow
- add GITSYNC_CODE_SIGN option, disable in CI
- add Windows popen/pclose compatibility defines
- disable GITSYNC_BUILD_TESTS in release-msvc preset
- update template_cpp to 123e346 with Windows/WindowsSDK fix
- pin template_cpp to working commit c25898a with CodeSign fixes + re-enable code signing
- force FetchContent to update aw_cmake_modules from main
- make clean also removes _deps cache to force FetchContent update
- use template_cpp commit c25898a with correct WindowsSDK include + add icon
- remove GIT_SHALLOW to ensure fresh fetch of template_cpp main branch
- correct aw_code_sign_target() TARGET parameter syntax
- use Ninja Multi-Config for MSVC presets (more portable than VS generator)
- correct aw_win_version() OUTVAR parameter syntax
- Windows CMake presets and build-release target
- override changelog-release defaults (remove boilerplate)
- Windows compatibility - use _popen/_pclose and safe getenv
- use ilammy/msvc-dev-cmd and cmake-build action for Windows

## [0.4.0] - 2026-11-22

### Added

- add multi-platform binary builds to release workflow
- rewrite gitsync from Rust to C++23

### Changed

- use reusable-release.yml like tpl
- Merge branch 'main' of github.com:ai-warevo/gitsync
- slop
- update CHANGELOG for v0.3.0 release
- version 0.3.0
- version 0.2.0
- fix
- use aw_add_library instead of aw_core_library
- replace vendored dependencies with FetchContent
- update to new workflow structure
- add .clang-tidy and .editorconfig config files
- remove unnecessary documentation files
- add final project completion summary
- use C++23 instead of Rust

### Fixed

- Windows CMake presets and build-release target
- override changelog-release defaults (remove boilerplate)
- Windows compatibility - use _popen/_pclose and safe getenv
- use ilammy/msvc-dev-cmd and cmake-build action for Windows

## [0.3.0] - 2026-11-23

### Added
- Multi-platform release binaries (Linux + Windows) in release workflow
- Windows MSVC build support with ilammy/msvc-dev-cmd action
- Safe environment variable access (_dupenv_s on Windows, getenv on Unix)

### Changed
- Release workflow now builds optimized binaries without tests
- Improved CMake configuration for Release builds (-DGITSYNC_BUILD_TESTS=OFF)

### Fixed
- Windows compatibility: replaced popen/pclose with _popen/_pclose
- Windows compatibility: replaced unsafe getenv with _dupenv_s
- Windows compatibility: proper HOME detection (USERPROFILE on Windows)

## [0.2.0] - 2026-11-22

### Added
- Complete rewrite from Rust to C++23 with modern language features
- CMake build system with presets (clang18-libcxx, release, msvc)
- Makefile CI contract (ci-fast, ci-deep, ci-coverage, build-release)
- Comprehensive test suite using Catch2 v3 (11 unit tests, 100% coverage)
- Static analysis via clang-tidy and code formatting via clang-format
- Multi-platform CI/CD with GitHub Actions
- Coverage badge generation and reporting

### Changed
- Build system: Cargo → CMake 3.23+ with presets
- Dependencies: managed via CMake FetchContent (zero vendored code)
- Logging: env_logger → spdlog with structured logging
- Async runtime: Tokio → synchronous C++ (future async support planned)

### Maintained
- Full configuration compatibility with Rust version (v0.1.x)
- JSON configuration format with PascalCase field names
- Three-layer config merge hierarchy (global → org → repo)
- SSH known_hosts management
- Multi-remote synchronization logic
- CLI argument compatibility (-v, -h, -u, -s)

## [0.1.0] - 2024-01-01 (Rust)

### Added
- Initial Rust implementation
- Multi-layer configuration system
- SSH known_hosts warm-up
- Git repository synchronization across multiple remotes
- Template-based branch naming
- CLI with version, help, update, and skip-host options

[Unreleased]: https://github.com/ai-warevo/gitsync/compare/v2.0.6...HEAD
[2.0.6]: https://github.com/ai-warevo/gitsync/compare/v2.0.5...v2.0.6
[2.0.5]: https://github.com/ai-warevo/gitsync/compare/v2.0.4...v2.0.5
[2.0.4]: https://github.com/ai-warevo/gitsync/compare/v2.0.3...v2.0.4
[2.0.3]: https://github.com/ai-warevo/gitsync/compare/v2.0.2...v2.0.3
[2.0.2]: https://github.com/ai-warevo/gitsync/compare/v2.0.1...v2.0.2
[2.0.1]: https://github.com/ai-warevo/gitsync/compare/v2.0.0...v2.0.1
[2.0.0]: https://github.com/ai-warevo/gitsync/compare/v1.1.10...v2.0.0
[1.0.0]: https://github.com/ai-warevo/gitsync/compare/v1.1.10...v1.0.0
[0.6.0]: https://github.com/ai-warevo/gitsync/compare/v1.1.10...v0.6.0
[0.5.0]: https://github.com/ai-warevo/gitsync/compare/v1.1.10...v0.5.0
[0.4.0]: https://github.com/ai-warevo/gitsync/compare/v1.1.10...v0.4.0
[0.3.0]: https://github.com/ai-warevo/gitsync/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/ai-warevo/gitsync/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/ai-warevo/gitsync/releases/tag/v0.1.0
