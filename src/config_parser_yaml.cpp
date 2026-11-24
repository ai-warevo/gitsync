#include "aw/gitsync/config_parser_yaml.hpp"

#include <fstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace aw::gitsync {

namespace {

/*
  Helpers that support both snake_case and PascalCase field names during deserialization.
  snake_case takes precedence if both are present.
*/

std::optional<std::string> get_string(const YAML::Node& node, const char* snake_name,
                                      const char* pascal_name) {
    if (node[snake_name] && node[snake_name].IsScalar()) {
        return node[snake_name].as<std::string>();
    }
    if (node[pascal_name] && node[pascal_name].IsScalar()) {
        return node[pascal_name].as<std::string>();
    }
    return std::nullopt;
}

std::optional<int> get_int(const YAML::Node& node, const char* snake_name,
                           const char* pascal_name) {
    if (node[snake_name] && node[snake_name].IsScalar()) {
        try {
            return node[snake_name].as<int>();
        } catch (...) {
        }
    }
    if (node[pascal_name] && node[pascal_name].IsScalar()) {
        try {
            return node[pascal_name].as<int>();
        } catch (...) {
        }
    }
    return std::nullopt;
}

std::optional<std::map<std::string, std::string>> get_object(const YAML::Node& node,
                                                               const char* snake_name,
                                                               const char* pascal_name) {
    if (node[snake_name] && node[snake_name].IsMap()) {
        std::map<std::string, std::string> result;
        for (const auto& pair : node[snake_name]) {
            if (pair.second.IsScalar()) {
                result[pair.first.as<std::string>()] = pair.second.as<std::string>();
            }
        }
        return result;
    }
    if (node[pascal_name] && node[pascal_name].IsMap()) {
        std::map<std::string, std::string> result;
        for (const auto& pair : node[pascal_name]) {
            if (pair.second.IsScalar()) {
                result[pair.first.as<std::string>()] = pair.second.as<std::string>();
            }
        }
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> get_array(const YAML::Node& node,
                                                   const char* snake_name,
                                                   const char* pascal_name) {
    if (node[snake_name] && node[snake_name].IsSequence()) {
        std::vector<std::string> result;
        for (const auto& val : node[snake_name]) {
            if (val.IsScalar()) {
                result.push_back(val.as<std::string>());
            }
        }
        return result;
    }
    if (node[pascal_name] && node[pascal_name].IsSequence()) {
        std::vector<std::string> result;
        for (const auto& val : node[pascal_name]) {
            if (val.IsScalar()) {
                result.push_back(val.as<std::string>());
            }
        }
        return result;
    }
    return std::nullopt;
}

} // namespace

PartialConfigSettings ConfigParserYaml::parse_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to read YAML config file: ") +
                                 path.string());
    }

    YAML::Node node;
    try {
        node = YAML::Load(file);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(std::string("Failed to parse YAML config: ") + e.what());
    }

    PartialConfigSettings result;
    result.log_level = get_string(node, "log_level", "LogLevel");
    result.remote_branch_template = get_string(node, "remote_branch_template", "RemoteBranchTemplate");
    result.source_remote_name = get_string(node, "source_remote_name", "SourceRemoteName");
    result.ssh_key_types = get_string(node, "ssh_key_types", "SshKeyTypes");
    result.remote_urls = get_object(node, "remote_urls", "RemoteUrls");
    result.max_retries = get_int(node, "max_retries", "MaxRetries");
    result.include_branches = get_array(node, "include_branches", "IncludeBranches");
    result.exclude_branches = get_array(node, "exclude_branches", "ExcludeBranches");
    result.include_remotes = get_array(node, "include_remotes", "IncludeRemotes");
    result.exclude_remotes = get_array(node, "exclude_remotes", "ExcludeRemotes");

    return result;
}

} // namespace aw::gitsync
