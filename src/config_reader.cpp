#include "aw/gitsync/config_reader.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>

#include "aw/gitsync/config_parser_json.hpp"
#include "aw/gitsync/config_parser_yaml.hpp"

namespace aw::gitsync {

namespace {
/*
  Wrapper for std::getenv with platform-specific secure variants.
  Uses _dupenv_s on Windows (safe) and std::getenv on Unix.
*/
std::optional<std::string> safe_getenv(const char* name) {
#ifdef _WIN32
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, name) == 0 && buffer != nullptr) {
        std::string result(buffer);
        free(buffer);
        return result;
    }
    return std::nullopt;
#else
    const char* value = std::getenv(name);
    if (value != nullptr) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}
} // namespace

ConfigSettings ConfigReader::read_config() {
    auto paths = collect_config_paths();

    if (paths.empty()) {
        throw std::runtime_error("Config file not found in any of the expected locations.");
    }

    PartialConfigSettings final_partial;

    for (const auto& path : paths) {
        auto cfg = read_config_from_path(path);
        final_partial.merge(cfg);
    }

    return final_partial.into_final_settings();
}

PartialConfigSettings ConfigReader::read_config_from_path(const std::filesystem::path& config_path) {
    /*
      Dispatch based on file extension: .yaml/.yml → YAML, else → JSON.
      Allows users to mix format (e.g., global JSON + repo YAML).
    */
    auto ext = config_path.extension();
    
    if (ext == ".yaml" || ext == ".yml") {
        return ConfigParserYaml::parse_file(config_path);
    }
    return ConfigParserJson::parse_file(config_path);
}

std::vector<std::filesystem::path> ConfigReader::collect_config_paths() {
    auto env_path = safe_getenv("GITSYNC_CONFIG_PATH");

    std::optional<std::filesystem::path> home_dir;
#ifdef _WIN32
    if (auto userprofile = safe_getenv("USERPROFILE")) {
        home_dir = std::filesystem::path(*userprofile);
    }
#else
    if (auto home = safe_getenv("HOME")) {
        home_dir = std::filesystem::path(*home);
    }
#endif

    std::optional<std::filesystem::path> current_dir;
    try {
        current_dir = std::filesystem::current_path();
    } catch (...) {
        /* Current directory may be inaccessible */
    }

    return resolve_config_hierarchy(env_path, home_dir, current_dir);
}

std::vector<std::filesystem::path> ConfigReader::resolve_config_hierarchy(
    const std::optional<std::string>& config_env,
    const std::optional<std::filesystem::path>& home_dir,
    const std::optional<std::filesystem::path>& current_dir) {
    std::vector<std::filesystem::path> paths;

    /*
      Hierarchy: GITSYNC_CONFIG_PATH (absolute override) →
      ~/.gitsync → ../.gitsync → ./.gitsync
    */
    if (config_env) {
        auto env_path = std::filesystem::path(*config_env);
        if (std::filesystem::exists(env_path)) {
            paths.push_back(env_path);
            return paths;
        }
    }

    if (home_dir) {
        auto json_path = *home_dir / ".gitsync" / "config.json";
        auto yaml_path = *home_dir / ".gitsync" / "config.yaml";
        auto yml_path = *home_dir / ".gitsync" / "config.yml";

        if (std::filesystem::exists(json_path)) {
            paths.push_back(json_path);
        } else if (std::filesystem::exists(yaml_path)) {
            paths.push_back(yaml_path);
        } else if (std::filesystem::exists(yml_path)) {
            paths.push_back(yml_path);
        }
    }

    if (current_dir) {
        if (auto parent = current_dir->parent_path(); parent != current_dir) {
            auto json_path = parent / ".gitsync" / "config.json";
            auto yaml_path = parent / ".gitsync" / "config.yaml";
            auto yml_path = parent / ".gitsync" / "config.yml";

            if (std::filesystem::exists(json_path)) {
                if (std::find(paths.begin(), paths.end(), json_path) == paths.end()) {
                    paths.push_back(json_path);
                }
            } else if (std::filesystem::exists(yaml_path)) {
                if (std::find(paths.begin(), paths.end(), yaml_path) == paths.end()) {
                    paths.push_back(yaml_path);
                }
            } else if (std::filesystem::exists(yml_path)) {
                if (std::find(paths.begin(), paths.end(), yml_path) == paths.end()) {
                    paths.push_back(yml_path);
                }
            }
        }

        auto json_path = *current_dir / ".gitsync" / "config.json";
        auto yaml_path = *current_dir / ".gitsync" / "config.yaml";
        auto yml_path = *current_dir / ".gitsync" / "config.yml";

        if (std::filesystem::exists(json_path)) {
            if (std::find(paths.begin(), paths.end(), json_path) == paths.end()) {
                paths.push_back(json_path);
            }
        } else if (std::filesystem::exists(yaml_path)) {
            if (std::find(paths.begin(), paths.end(), yaml_path) == paths.end()) {
                paths.push_back(yaml_path);
            }
        } else if (std::filesystem::exists(yml_path)) {
            if (std::find(paths.begin(), paths.end(), yml_path) == paths.end()) {
                paths.push_back(yml_path);
            }
        }
    }

    return paths;
}

} // namespace aw::gitsync
