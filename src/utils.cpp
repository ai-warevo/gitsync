#include "aw/gitsync/utils.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace aw::gitsync {

namespace {

/*
  Parse semantic version strings (X.Y.Z[...]) into components.
  Supports leading 'v' prefix (v1.2.3).
*/
std::optional<std::vector<int>> parse_version_parts(std::string_view version) {
    std::vector<int> parts;
    std::string_view remaining = version;

    if (!remaining.empty() && remaining[0] == 'v') {
        remaining.remove_prefix(1);
    }

    while (!remaining.empty()) {
        auto dot_pos = remaining.find('.');
        std::string_view part = (dot_pos == std::string_view::npos) ? remaining : remaining.substr(0, dot_pos);

        int value = 0;
        auto [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), value);

        if (ec != std::errc{}) {
            return std::nullopt;
        }

        parts.push_back(value);

        if (dot_pos == std::string_view::npos) {
            break;
        }

        remaining.remove_prefix(dot_pos + 1);
    }

    return parts;
}

}  // namespace

std::string parse_latest_version(std::string_view json_response, std::string_view current_version) {
    /*
      Extract "tag_name" value from GitHub API release JSON response.
      Falls back to plain-text content (trimmed) for raw VERSION files.
      Falls back to current_version if neither works.
    */
    constexpr std::string_view tag_name_key = R"("tag_name")";
    auto pos = json_response.find(tag_name_key);

    if (pos != std::string_view::npos) {
        auto colon_pos = json_response.find(':', pos + tag_name_key.size());
        if (colon_pos != std::string_view::npos) {
            auto quote_pos = json_response.find('"', colon_pos);
            if (quote_pos != std::string_view::npos) {
                auto end = json_response.find('"', quote_pos + 1);
                if (end != std::string_view::npos) {
                    return std::string(json_response.substr(quote_pos + 1, end - quote_pos - 1));
                }
            }
        }
    }

    /* Plain-text fallback: trim whitespace, verify it looks like a version */
    auto start = json_response.find_first_not_of(" \t\r\n");
    auto end = json_response.find_last_not_of(" \t\r\n");
    if (start != std::string_view::npos && end != std::string_view::npos) {
        std::string trimmed = std::string(json_response.substr(start, end - start + 1));
        /* Accept plain version strings like "2.0.20" or "v2.0.20" */
        if (parse_version_parts(trimmed)) {
            return trimmed;
        }
    }

    return std::string(current_version);
}

int compare_versions(std::string_view v1, std::string_view v2) {
    /*
      Lexicographic comparison of semantic versions.
      Returns: >0 if v1>v2, <0 if v1<v2, 0 if equal, -2 on parse error.
    */
    auto parts1 = parse_version_parts(v1);
    auto parts2 = parse_version_parts(v2);

    if (!parts1 || !parts2) {
        return -2;
    }

    size_t min_size = std::min(parts1->size(), parts2->size());

    for (size_t i = 0; i < min_size; ++i) {
        if ((*parts1)[i] > (*parts2)[i]) {
            return 1;
        }
        if ((*parts1)[i] < (*parts2)[i]) {
            return -1;
        }
    }

    if (parts1->size() > parts2->size()) {
        return 1;
    }
    if (parts1->size() < parts2->size()) {
        return -1;
    }

    return 0;
}

std::optional<std::string> find_asset_url(std::string_view json_response, std::string_view asset_name) {
    spdlog::debug("find_asset_url: response size {} bytes", json_response.size());
    
    try {
        auto json = nlohmann::json::parse(json_response);
        
        if (!json.contains("assets")) {
            spdlog::warn("find_asset_url: 'assets' key not found in JSON");
            return std::nullopt;
        }
        
        if (!json["assets"].is_array()) {
            spdlog::warn("find_asset_url: 'assets' is not an array, type: {}", json["assets"].type_name());
            return std::nullopt;
        }
        
        spdlog::debug("find_asset_url: assets array has {} items", json["assets"].size());
        spdlog::debug("find_asset_url: searching for asset '{}'", asset_name);
        
        for (const auto& asset : json["assets"]) {
            if (!asset.contains("name")) {
                spdlog::debug("find_asset_url: asset missing 'name' field");
                continue;
            }
            
            if (!asset["name"].is_string()) {
                spdlog::debug("find_asset_url: asset 'name' is not string");
                continue;
            }
            
            std::string name = asset["name"].get<std::string>();
            spdlog::debug("find_asset_url: found asset '{}'", name);
            
            if (name == asset_name) {
                spdlog::debug("find_asset_url: asset name matches, looking for browser_download_url");
                
                if (!asset.contains("browser_download_url")) {
                    spdlog::warn("find_asset_url: asset '{}' found but 'browser_download_url' key missing", asset_name);
                    return std::nullopt;
                }
                
                if (!asset["browser_download_url"].is_string()) {
                    spdlog::warn("find_asset_url: asset '{}' 'browser_download_url' is not string", asset_name);
                    return std::nullopt;
                }
                
                std::string url = asset["browser_download_url"].get<std::string>();
                spdlog::info("find_asset_url: extracted download URL for '{}': {}", asset_name, url);
                return url;
            }
        }
        
        spdlog::warn("find_asset_url: asset '{}' not found in assets array", asset_name);
        return std::nullopt;
    } catch (const std::exception& e) {
        spdlog::warn("find_asset_url: JSON parse error: {}", e.what());
        return std::nullopt;
    }
}

std::string get_update_file_name() {
#ifdef _WIN32
    return std::string(UPDATE_FILE_BASE) + ".exe";
#else
    return std::string(UPDATE_FILE_BASE);
#endif
}

}  // namespace aw::gitsync

