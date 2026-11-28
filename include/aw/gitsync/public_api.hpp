/// Public API Stabilization for gitsync v1.0+
///
/// This header declares all stable, public-facing APIs for gitsync v1.0.0 and later.
/// These APIs are guaranteed to remain stable within v1.x (backward compatible).
///
/// Stable APIs (guaranteed through v1.x):
///   - ConfigSettings (data container)
///   - PartialConfigSettings (data container for merging)
///   - ConfigReader::read_config()
///   - ConfigReader::read_config_from_path()
///   - ConfigReader::collect_config_paths()
///   - GitSyncService::sanitize_branch_name()
///   - GitSyncService::build_remote_branch()
///   - GitSyncService::perform_sync()
///
/// Internal APIs (may change without notice):
///   - KnownHostsService (internal utility)
///   - ProcessRunner (internal utility)
///   - utils.hpp (internal helpers)
///
/// Version Policy:
///   - v1.0.0 to v1.x: Stable APIs guaranteed backward compatible
///   - v2.0.0+: May introduce breaking changes
///   - Patch releases (v1.0.1, etc): Bug fixes only, no API changes
///
/// Library consumers should:
///   1. Include only public headers from include/aw/gitsync/
///   2. Use only ConfigSettings, ConfigReader, GitSyncService
///   3. NOT depend on KnownHostsService or internal headers
///   4. Report breaking changes if found in v1.x (bug)

#ifndef AW_GITSYNC_PUBLIC_API_HPP
#define AW_GITSYNC_PUBLIC_API_HPP

#include "aw/gitsync/config_reader.hpp"
#include "aw/gitsync/config_settings.hpp"
#include "aw/gitsync/git_sync_service.hpp"

namespace aw::gitsync {

/// Namespace: aw::gitsync (stable, v1.0+)
/// All public APIs reside in aw::gitsync namespace.

/// Configuration container (stable v1.0+)
/// Use ConfigReader to populate this struct.
/// Fields are read-only after creation; merge configurations before instantiation.
using StableConfigSettings = ConfigSettings;

/// Configuration reader (stable v1.0+)
/// Entry point for all configuration loading:
///   auto settings = ConfigReader::read_config();
///   auto partial = ConfigReader::read_config_from_path(path);
///   auto paths = ConfigReader::collect_config_paths();
using StableConfigReader = ConfigReader;

/// Git synchronization service (stable v1.0+)
/// Entry point for all sync operations:
///   GitSyncService service(settings);
///   service.perform_sync();
using StableGitSyncService = GitSyncService;

} // namespace aw::gitsync

/// Recommended usage pattern for v1.0+:
///
/// #include <aw/gitsync/public_api.hpp>
/// #include <spdlog/spdlog.h>
///
/// int main() {
///     try {
///         auto settings = aw::gitsync::ConfigReader::read_config();
///         auto service = aw::gitsync::GitSyncService(settings);
///         service.perform_sync();
///     } catch (const std::exception& e) {
///         spdlog::error("Sync failed: {}", e.what());
///         return 1;
///     }
///     return 0;
/// }

#endif // AW_GITSYNC_PUBLIC_API_HPP
