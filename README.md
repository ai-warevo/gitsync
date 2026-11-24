# aw::gitsync

[![CI](https://github.com/ai-warevo/gitsync/actions/workflows/ci.yml/badge.svg)](https://github.com/ai-warevo/gitsync/actions/workflows/ci.yml)
[![Coverage](https://raw.githubusercontent.com/ai-warevo/gitsync/badges/coverage.svg)](https://github.com/ai-warevo/gitsync)

A modern C++23 utility for synchronizing Git repositories across multiple remotes.

## Features

- **Multi-layer configuration**: Global → Organization → Repository config hierarchy
- **SSH known_hosts management**: Automatic warm-up of target remote hosts
- **Flexible mirroring**: Template-based branch naming with `%owner%/%reponame%/%branchname%`
- **Cross-platform**: Linux and Windows support

## Requirements

- C++23 compiler (Clang 18+ with libc++ or MSVC on Windows)
- CMake 3.23+
- Git

## Building

```bash
# Configure and build
cmake --preset clang18-libcxx
cmake --build --preset clang18-libcxx

# Run tests
ctest --preset clang18-libcxx --output-on-failure

# Build release binary
make build-release
# Output: dist/gitsync
```

### Makefile Targets

```bash
make build       # Configure and build with current preset
make test        # Run all tests
make lint        # Run clang-format and clang-tidy checks
make clean       # Remove build artifacts

# CI targets
make ci-fast     # Fast PR checks: lint + build + tests
make ci-deep     # Deep checks: sanitizers + integration tests
make ci-coverage # Generate coverage report and badge
make build-release # Build optimized release artifacts
```

## Usage

```bash
# Run sync (reads config from .gitsync/config.json)
gitsync

# Skip SSH known_hosts warm-up
gitsync --skip-host

# Show version
gitsync --version

# Show help
gitsync --help

# Check for updates
gitsync --update
```

## Configuration

See [docs/README.md](docs/README.md) for detailed configuration documentation.

### Quick Start

Create `~/.gitsync/config.json`:

```json
{
  "LogLevel": "info",
  "SourceRemoteName": "origin",
  "RemoteBranchTemplate": "%owner%/%reponame%/%branchname%",
  "RemoteUrls": {
    "gitlab": "git@gitlab.com:username/mirror.git",
    "backup": "git@github.com:username/backup.git"
  }
}
```

### Configuration Hierarchy

1. **Global** (`~/.gitsync/config.json`): Base settings for all repositories
2. **Organization** (`../.gitsync/config.json`): Shared settings for project group
3. **Repository** (`./.gitsync/config.json`): Project-specific overrides

Each level supplements or overrides the previous one.

### Environment Override

```bash
GITSYNC_CONFIG_PATH=/path/to/custom/config.json gitsync
```

## Configuration Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `LogLevel` | string | `"info"` | Logging verbosity (`trace`, `debug`, `info`, `warn`, `error`) |
| `SourceRemoteName` | string | `"origin"` | Source git remote name |
| `RemoteBranchTemplate` | string | `"%owner%/%reponame%/%branchname%"` | Branch naming template |
| `RemoteUrls` | object | `{}` | Target mirror URLs (name → URL mapping) |
| `SshKeyTypes` | string | `""` | Comma-separated SSH key types for `ssh-keyscan` (e.g., `"ed25519,rsa"`) |

## Development

### Project Structure

```
gitsync/
├── CMakeLists.txt          # CMake configuration
├── CMakePresets.json       # Build presets (clang18-libcxx, release, msvc)
├── Makefile                # CI contract targets
├── include/aw/gitsync/     # Public headers
│   ├── config_settings.hpp
│   ├── config_reader.hpp
│   ├── process_runner.hpp
│   ├── known_hosts_service.hpp
│   └── git_sync_service.hpp
├── src/                    # Implementation
│   ├── main.cpp
│   ├── config_settings.cpp
│   ├── config_reader.cpp
│   ├── process_runner.cpp
│   ├── known_hosts_service.cpp
│   └── git_sync_service.cpp
├── tests/                  # Catch2 tests
│   └── gitsync_tests.cpp
├── .rust-archive/          # Original Rust implementation (reference)
└── docs/                   # Documentation
```

### Dependencies

- [nlohmann/json](https://github.com/nlohmann/json) v3.11.3 - JSON parsing
- [spdlog](https://github.com/gabime/spdlog) v1.14.1 - Logging
- [Catch2](https://github.com/catchorg/Catch2) v3.7.1 - Testing

All dependencies are fetched via CMake `FetchContent`.

### CI/CD

- **PR checks**: Fast lint, build, and tests via `reusable-pr-fast.yml`
- **Weekly deep**: Sanitizers and integration tests via `reusable-weekly-deep.yml`
- **Coverage**: Badge generation on main branch push
- **Release**: Tag `vX.Y.Z` triggers multi-platform builds (Linux + Windows) and GitHub Release creation

### Releases

Tag a version and push to trigger release workflow:

```bash
git tag v1.0.0
git push origin v1.0.0
```

Release artifacts:
- `gitsync` (Linux binary)
- `gitsync.exe` (Windows binary)

## Migration from Rust

This is a C++23 rewrite of the original Rust implementation. The Rust source is preserved in `.rust-archive/` for reference.

### Key Changes

- **Language**: Rust → C++23
- **Build system**: Cargo → CMake with presets
- **Dependencies**: Managed via CMake FetchContent instead of Cargo.toml
- **Async**: Tokio async/await → synchronous C++ with optional future async support
- **Configuration**: JSON parsing via nlohmann/json (same PascalCase format)
- **Logging**: env_logger → spdlog

### Compatibility

Configuration files are fully compatible. The JSON format and three-layer merge logic are preserved.

## License

MIT

## Contributing

1. Fork the repository to your account
2. Create a feature branch: `git checkout -b feat/my-feature`
3. Make changes and commit: `git commit -m "feat: add my feature"`
4. Push to your fork: `git push origin feat/my-feature`
5. Create a Pull Request to `ai-warevo/gitsync:main`

PRs require:
- ✅ All tests passing
- ✅ Lint checks passing (clang-format + clang-tidy)
- ✅ Code review approval
