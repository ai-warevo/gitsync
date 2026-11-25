#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#    include <windows.h>
#endif

#include <spdlog/spdlog.h>

#include "aw/gitsync/cli_args.hpp"
#include "aw/gitsync/config_reader.hpp"
#include "aw/gitsync/git_sync_service.hpp"
#include "aw/gitsync/known_hosts_service.hpp"
#include "aw/gitsync/process_runner.hpp"
#include "aw/gitsync/self_update.hpp"
#include "aw/gitsync/utils.hpp"

// Platform-specific popen/pclose
#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

// Version is provided at compile time via CMake
#ifndef GITSYNC_VERSION
    #define GITSYNC_VERSION "0.0.0"
#endif

using namespace aw::gitsync;

void init_logging(const std::string& log_level) {
    /*
      Map config log_level strings to spdlog levels.
      Defaults to info if unrecognized.
    */
    if (log_level == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (log_level == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (log_level == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (log_level == "err" || log_level == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
}

void run_app(const CliArgs& cli_args, bool skip_host) {
    auto config = ConfigReader::read_config();
    
    if (auto override = cli_args.log_level_override()) {
        config.log_level = *override;
    }
    
    init_logging(config.log_level);

    auto process_runner = std::make_shared<ProcessRunner>();
    auto known_hosts_service = std::make_unique<KnownHostsService>(process_runner, config);
    auto git_sync_service = std::make_unique<GitSyncService>(process_runner, config);

    if (!skip_host) {
        known_hosts_service->warm_up();
    }

    if (cli_args.dry_run) {
        spdlog::info("Running in dry-run mode (no changes will be made)");
        spdlog::debug("Current branch: (would be detected here)");
        spdlog::debug("Source remote: {}", config.source_remote_name);
        spdlog::debug("Will sync to remotes:");
        for (const auto& [name, url] : config.remote_urls) {
            spdlog::debug("  - {}: {}", name, url);
        }
        if (!config.exclude_remotes.empty()) {
            spdlog::debug("Will exclude remotes:");
            for (const auto& remote : config.exclude_remotes) {
                spdlog::debug("  - {}", remote);
            }
        }
        spdlog::info("Dry-run complete (exit code: 0)");
        return;
    }

    git_sync_service->run();
}

void print_version() {
    std::cout << "gitsync v" << GITSYNC_VERSION << '\n';
}

/*
  Extract download URL for a specific asset from GitHub release JSON.
  Returns std::nullopt if the asset is not found.
*/
std::optional<std::string> find_asset_url(std::string_view json_response, std::string_view asset_name) {
    constexpr std::string_view assets_key = R"("assets":[)";
    auto assets_pos = json_response.find(assets_key);
    if (assets_pos == std::string_view::npos) {
        return std::nullopt;
    }

    std::string_view assets_section = json_response.substr(assets_pos + assets_key.size());

    std::string search_pattern = std::format(R"("name":"{}")", asset_name);
    auto name_pos = assets_section.find(search_pattern);
    if (name_pos == std::string_view::npos) {
        return std::nullopt;
    }

    std::string_view after_name = assets_section.substr(name_pos);
    
    /* Find browser_download_url within this asset object (stops at next asset or end of array) */
    constexpr std::string_view url_key = R"("browser_download_url":")";
    auto url_pos = after_name.find(url_key);
    if (url_pos == std::string_view::npos) {
        return std::nullopt;
    }

    auto url_start = url_pos + url_key.size();
    auto url_end = after_name.find('"', url_start);
    if (url_end == std::string_view::npos) {
        return std::nullopt;
    }

    return std::string(after_name.substr(url_start, url_end - url_start));
}

void check_for_update(const ConfigSettings& config) {
    std::cout << "gitsync v" << GITSYNC_VERSION << '\n';
    std::cout << "Checking for updates...\n";

    std::string asset_name;
#ifdef _WIN32
    asset_name = "gitsync.exe";
#else
    asset_name = "gitsync";
#endif

    /* Fetch release info from configured URL (defaults to GitHub main branch) */
    const std::string& url = config.update_check_url;
    const std::string curl_cmd = std::format(R"(curl -s "{}")", url);

    FILE* pipe = popen(curl_cmd.c_str(), "r");
    if (!pipe) {
        spdlog::warn("Failed to run curl for update check");
        return;
    }

    std::string response;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }

    int status = pclose(pipe);
    if (status != 0) {
        spdlog::warn("Failed to fetch release info from: {}", url);
        return;
    }

    if (response.empty()) {
        spdlog::warn("Empty response from update check");
        return;
    }

    /* Parse latest version from GitHub API response */
    std::string latest_version = parse_latest_version(response, GITSYNC_VERSION);

    std::string_view current = GITSYNC_VERSION;
    std::string_view latest = latest_version;
    if (current.starts_with('v')) {
        current.remove_prefix(1);
    }
    if (latest.starts_with('v')) {
        latest.remove_prefix(1);
    }

    int cmp = compare_versions(current, latest);
    if (cmp >= 0) {
        std::cout << "Already up to date (v" << GITSYNC_VERSION << ")\n";
        return;
    }

    std::cout << std::format("New version available: v{} (current: v{})\n", latest, GITSYNC_VERSION);

    /* Find download URL for the asset */
    auto asset_url = find_asset_url(response, asset_name);
    if (!asset_url) {
        spdlog::warn("Failed to find download URL for asset: {}", asset_name);
        std::exit(1);
    }

    std::cout << "Downloading update...\n";

    /* Get current executable path */
    auto [current_exe, err] = get_self_path();
    if (err) {
        spdlog::warn("Failed to get current executable path: {}", err->msg);
        std::exit(1);
    }

    /* Download to parent directory for staged update */
    std::filesystem::path parent_dir = current_exe.parent_path();
#ifdef _WIN32
    std::filesystem::path update_file = parent_dir / "gitsync-update.exe";
#else
    std::filesystem::path update_file = parent_dir / "gitsync-update";
#endif

    std::string download_cmd = std::format(R"(curl -L -o "{}" "{}")", update_file.string(), *asset_url);
    int download_status = std::system(download_cmd.c_str());
    if (download_status != 0) {
        spdlog::warn("Download failed (curl exit code: {})", download_status);
        std::exit(1);
    }

    /* Perform self-update (replace running binary, restart with --cleanup) */
    auto [ok, update_err] = self_update(current_exe, "gitsync-update");
    if (!ok) {
        spdlog::error("Self-update failed: {}", update_err ? update_err->msg : "unknown error");
        std::exit(1);
    }

    std::cout << std::format("Updated to v{}\n", latest);
}

void print_help() {
    std::cout << "gitsync v" << GITSYNC_VERSION << "\n\n";
    std::cout << "USAGE:\n";
    std::cout << "    gitsync [OPTIONS]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "    -d, --dry-run       Preview sync without pushing (no changes made)\n";
    std::cout << "    -vv, --verbose      Increase verbosity to DEBUG level\n";
    std::cout << "    -q, --quiet         Decrease verbosity to ERROR level\n";
    std::cout << "    --json              Output results as JSON (for CI/CD integration)\n";
    std::cout << "    -h, --help          Print help\n";
    std::cout << "    -v, --version       Print version\n";
    std::cout << "    -u, --update        Check for and apply updates\n";
    std::cout << "    -s, --skip-host     Skip SSH known_hosts warm-up\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "    gitsync --dry-run           Preview what would be synced\n";
    std::cout << "    gitsync --verbose           Sync with detailed logging\n";
    std::cout << "    gitsync --dry-run --json    Preview as JSON output\n";
}

int main(int argc, char* argv[]) {
    try {
        std::vector<std::string> args;
        args.reserve(static_cast<size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }

        CliArgs cli_args = parse_cli_args(args);

        if (cli_args.help) {
            print_help();
            return 0;
        }

        if (cli_args.version) {
            print_version();
            return 0;
        }

        if (cli_args.update) {
            auto config = ConfigReader::read_config();
            check_for_update(config);
            return 0;
        }

        if (cli_args.cleanup) {
            auto [current_exe, err] = get_self_path();
            if (!err) {
                cleanup_old_binary(current_exe);
            }
        }

        run_app(cli_args, cli_args.skip_host);
        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
