#include "aw/gitsync/config_parser_yaml.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace aw::gitsync {

namespace {

/* Minimal YAML-to-JSON converter for flat config files.
   Handles scalars, maps, and sequences. No nested structures. */
nlohmann::json yaml_to_json(std::istream& input) {
    nlohmann::json root;
    std::string line;
    std::string current_key;
    std::vector<std::string> current_sequence;
    bool in_sequence = false;

    while (std::getline(input, line)) {
        /* Skip empty lines and comments */
        if (line.empty() || line[0] == '#') {
            continue;
        }

        /* Calculate indentation */
        size_t indent_pos = line.find_first_not_of(" \t");
        if (indent_pos == std::string::npos) {
            continue;
        }

        int current_indent = indent_pos;
        std::string content = line.substr(indent_pos);

        /* Flush sequence if we decreased indent */
        if (in_sequence && current_indent == 0) {
            if (!current_sequence.empty()) {
                root[current_key] = current_sequence;
                current_sequence.clear();
            }
            in_sequence = false;
        }

        /* Handle sequence items (lines starting with -) */
        if (content[0] == '-') {
            in_sequence = true;
            std::string item = content.substr(1);
            /* Strip leading/trailing whitespace and quotes */
            item.erase(0, item.find_first_not_of(" \t\"'"));
            item.erase(item.find_last_not_of(" \t\"'") + 1);
            current_sequence.push_back(item);
            continue;
        }

        /* Handle key-value pairs */
        size_t colon_pos = content.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        /* Flush any pending sequence */
        if (in_sequence && !current_sequence.empty()) {
            root[current_key] = current_sequence;
            current_sequence.clear();
            in_sequence = false;
        }

        std::string key = content.substr(0, colon_pos);
        key.erase(key.find_last_not_of(" \t") + 1);
        current_key = key;

        std::string value = content.substr(colon_pos + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        /* Skip empty values or nested structures */
        if (value.empty() || value == "{" || value == "[") {
            continue;
        }

        /* Strip quotes */
        if ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\'')) {
            value = value.substr(1, value.size() - 2);
        }

        /* Try to parse as number */
        try {
            if (value.find('.') != std::string::npos) {
                root[key] = std::stod(value);
            } else {
                root[key] = std::stoi(value);
            }
        } catch (...) {
            /* Store as string */
            root[key] = value;
        }
    }

    /* Flush any remaining sequence */
    if (in_sequence && !current_sequence.empty()) {
        root[current_key] = current_sequence;
    }

    return root;
}

std::optional<std::string> get_string(const nlohmann::json& obj, const char* snake_name,
                                      const char* pascal_name) {
    if (obj.contains(snake_name) && obj[snake_name].is_string()) {
        return obj[snake_name].get<std::string>();
    }
    if (obj.contains(pascal_name) && obj[pascal_name].is_string()) {
        return obj[pascal_name].get<std::string>();
    }
    return std::nullopt;
}

std::optional<int> get_int(const nlohmann::json& obj, const char* snake_name,
                           const char* pascal_name) {
    if (obj.contains(snake_name) && obj[snake_name].is_number_integer()) {
        return obj[snake_name].get<int>();
    }
    if (obj.contains(pascal_name) && obj[pascal_name].is_number_integer()) {
        return obj[pascal_name].get<int>();
    }
    return std::nullopt;
}

std::optional<std::map<std::string, std::string>> get_object(const nlohmann::json& obj,
                                                              const char* snake_name,
                                                              const char* pascal_name) {
    const nlohmann::json* target = nullptr;
    if (obj.contains(snake_name) && obj[snake_name].is_object()) {
        target = &obj[snake_name];
    } else if (obj.contains(pascal_name) && obj[pascal_name].is_object()) {
        target = &obj[pascal_name];
    }

    if (target) {
        std::map<std::string, std::string> result;
        for (const auto& [k, v] : target->items()) {
            if (v.is_string()) {
                result[k] = v.get<std::string>();
            }
        }
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> get_array(const nlohmann::json& obj,
                                                   const char* snake_name,
                                                   const char* pascal_name) {
    const nlohmann::json* target = nullptr;
    if (obj.contains(snake_name) && obj[snake_name].is_array()) {
        target = &obj[snake_name];
    } else if (obj.contains(pascal_name) && obj[pascal_name].is_array()) {
        target = &obj[pascal_name];
    }

    if (target) {
        std::vector<std::string> result;
        for (const auto& val : *target) {
            if (val.is_string()) {
                result.push_back(val.get<std::string>());
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

    nlohmann::json node;
    try {
        node = yaml_to_json(file);
    } catch (const std::exception& e) {
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

