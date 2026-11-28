#ifndef AW_GITSYNC_CONFIG_PARSER_COMMON_HPP
#define AW_GITSYNC_CONFIG_PARSER_COMMON_HPP

#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace aw::gitsync {

struct ConfigField;

/** Extract string value from JSON for a config field.
    Tries both PascalCase (json_key) and snake_case field names. */
std::optional<std::string> parse_config_string(const nlohmann::json& j, const ConfigField& field);

} // namespace aw::gitsync

#endif // AW_GITSYNC_CONFIG_PARSER_COMMON_HPP
