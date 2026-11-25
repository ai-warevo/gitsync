#include "aw/gitsync/config_parser_json.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "aw/gitsync/config_parser_common.hpp"
#include "aw/gitsync/config_schema.hpp"

using json = nlohmann::json;

namespace aw::gitsync {

namespace {

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

std::optional<int> get_int(const json& j, const char* snake_name, const char* pascal_name) {
    if (j.contains(snake_name) && j[snake_name].is_number_integer()) {
        return j[snake_name].get<int>();
    }
    if (j.contains(pascal_name) && j[pascal_name].is_number_integer()) {
        return j[pascal_name].get<int>();
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
    
    /* Parse simple string fields using schema */
    for (const auto& field : CONFIG_SCHEMA) {
        if (auto value = parse_config_string(j, field)) {
            if (field.name == "log_level") {
                result.log_level = *value;
            } else if (field.name == "remote_branch_template") {
                result.remote_branch_template = *value;
            } else if (field.name == "source_remote_name") {
                result.source_remote_name = *value;
            } else if (field.name == "ssh_key_types") {
                result.ssh_key_types = *value;
            } else if (field.name == "update_check_url") {
                result.update_check_url = *value;
            } else if (field.name == "release_download_url_template") {
                result.release_download_url_template = *value;
            }
        }
    }
    
    /* Parse complex types (arrays, objects) manually */
    result.remote_urls = get_object(j, "remote_urls", "RemoteUrls");
    result.max_retries = get_int(j, "max_retries", "MaxRetries");
    result.include_branches = get_array(j, "include_branches", "IncludeBranches");
    result.exclude_branches = get_array(j, "exclude_branches", "ExcludeBranches");
    result.include_remotes = get_array(j, "include_remotes", "IncludeRemotes");
    result.exclude_remotes = get_array(j, "exclude_remotes", "ExcludeRemotes");

    return result;
}

} // namespace aw::gitsync
