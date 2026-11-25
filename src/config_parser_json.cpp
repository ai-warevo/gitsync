#include "aw/gitsync/config_parser_json.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace aw::gitsync {

namespace {

/*
  Helpers that support both snake_case and PascalCase field names during deserialization.
  snake_case takes precedence if both are present.
*/

std::optional<std::string> get_string(const json& j, const char* snake_name,
                                      const char* pascal_name) {
    if (j.contains(snake_name) && j[snake_name].is_string()) {
        return j[snake_name].get<std::string>();
    }
    if (j.contains(pascal_name) && j[pascal_name].is_string()) {
        return j[pascal_name].get<std::string>();
    }
    return std::nullopt;
}

std::optional<int> get_int(const json& j, const char* snake_name, const char* pascal_name) {
    if (j.contains(snake_name) && j[snake_name].is_number_integer()) {
        return j[snake_name].get<int>();
    }
    if (j.contains(pascal_name) && j[pascal_name].is_number_integer()) {
        return j[pascal_name].get<int>();
    }
    return std::nullopt;
}

std::optional<std::map<std::string, std::string>> get_object(const json& j,
                                                               const char* snake_name,
                                                               const char* pascal_name) {
    if (j.contains(snake_name) && j[snake_name].is_object()) {
        std::map<std::string, std::string> result;
        for (const auto& [key, val] : j[snake_name].items()) {
            if (val.is_string()) {
                result[key] = val.get<std::string>();
            }
        }
        return result;
    }
    if (j.contains(pascal_name) && j[pascal_name].is_object()) {
        std::map<std::string, std::string> result;
        for (const auto& [key, val] : j[pascal_name].items()) {
            if (val.is_string()) {
                result[key] = val.get<std::string>();
            }
        }
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> get_array(const json& j, const char* snake_name,
                                                   const char* pascal_name) {
    if (j.contains(snake_name) && j[snake_name].is_array()) {
        std::vector<std::string> result;
        for (const auto& val : j[snake_name]) {
            if (val.is_string()) {
                result.push_back(val.get<std::string>());
            }
        }
        return result;
    }
    if (j.contains(pascal_name) && j[pascal_name].is_array()) {
        std::vector<std::string> result;
        for (const auto& val : j[pascal_name]) {
            if (val.is_string()) {
                result.push_back(val.get<std::string>());
            }
        }
        return result;
    }
    return std::nullopt;
}

} // namespace

PartialConfigSettings ConfigParserJson::parse_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to read JSON config file: ") +
                                 path.string());
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("Failed to parse JSON config: ") + e.what());
    }

    PartialConfigSettings result;
    result.log_level = get_string(j, "log_level", "LogLevel");
    result.remote_branch_template = get_string(j, "remote_branch_template", "RemoteBranchTemplate");
    result.source_remote_name = get_string(j, "source_remote_name", "SourceRemoteName");
    result.ssh_key_types = get_string(j, "ssh_key_types", "SshKeyTypes");
    result.update_check_url = get_string(j, "update_check_url", "UpdateCheckUrl");
    result.release_download_url_template = get_string(j, "release_download_url_template", "ReleaseDownloadUrlTemplate");
    result.remote_urls = get_object(j, "remote_urls", "RemoteUrls");
    result.max_retries = get_int(j, "max_retries", "MaxRetries");
    result.include_branches = get_array(j, "include_branches", "IncludeBranches");
    result.exclude_branches = get_array(j, "exclude_branches", "ExcludeBranches");
    result.include_remotes = get_array(j, "include_remotes", "IncludeRemotes");
    result.exclude_remotes = get_array(j, "exclude_remotes", "ExcludeRemotes");

    return result;
}

} // namespace aw::gitsync
