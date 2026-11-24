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
    auto end_brace = after_name.find("}}");
    if (end_brace == std::string_view::npos) {
        return std::nullopt;
    }

    std::string_view asset_obj = after_name.substr(0, end_brace + 2);

    constexpr std::string_view url_key = R"("browser_download_url":")";
    auto url_pos = asset_obj.find(url_key);
    if (url_pos == std::string_view::npos) {
        return std::nullopt;
    }

    auto url_start = url_pos + url_key.size();
    auto url_end = asset_obj.find('"', url_start);
    if (url_end == std::string_view::npos) {
        return std::nullopt;
    }

    return std::string(asset_obj.substr(url_start, url_end - url_start));
}

void check_for_update() {
    std::cout << "gitsync v" << GITSYNC_VERSION << '\n';
    std::cout << "Checking for updates...\n";

    std::string asset_name;
#ifdef _WIN32
    asset_name = "gitsync.exe";
#else
    asset_name = "gitsync";
#endif

    const std::string url = "https://api.github.com/repos/ai-warevo/gitsync/releases/latest";
    const std::string curl_cmd =
        std::format(R"(curl -s -H "Accept: application/vnd.github+json" "{}")", url);

    FILE* pipe = popen(curl_cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run curl\n";
        std::exit(1);
    }

    std::string response;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }

    int status = pclose(pipe);
    if (status != 0) {
        std::cerr << "Failed to fetch release info\n";
        std::exit(1);
    }

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

    auto asset_url = find_asset_url(response, asset_name);
    if (!asset_url) {
        std::cerr << std::format("Could not find download URL for asset: {}\n", asset_name);
        std::exit(1);
    }

    std::cout << "Downloading update...\n";

    std::filesystem::path current_exe;
    try {
#ifdef _WIN32
        char exe_buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_buffer, MAX_PATH);
        current_exe = exe_buffer;
#else
        current_exe = std::filesystem::read_symlink("/proc/self/exe");
#endif
    } catch (const std::exception& e) {
        std::cerr << std::format("Failed to get current executable path: {}\n", e.what());
        std::exit(1);
    }

    std::string download_cmd = std::format(R"(curl -L -o "{}" "{}")", current_exe.string(), *asset_url);
    int download_status = std::system(download_cmd.c_str());
    if (download_status != 0) {
        std::cerr << "Download failed\n";
        std::exit(1);
    }

#ifndef _WIN32
    std::string chmod_cmd = std::format("chmod +x \"{}\"", current_exe.string());
    std::system(chmod_cmd.c_str());
#endif

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
        args.reserve(argc);
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
            check_for_update();
            return 0;
        }

        run_app(cli_args, cli_args.skip_host);
        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
