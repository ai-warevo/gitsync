#include "aw/gitsync/config_parser_common.hpp"

#include <string>

#include "aw/gitsync/config_schema.hpp"

namespace aw::gitsync {

std::optional<std::string> parse_config_string(const nlohmann::json& j, const ConfigField& field) {
    /* Try PascalCase key first */
    std::string json_key{field.json_key};
    if (j.contains(json_key)) {
        const auto& val = j[json_key];
        if (val.is_string()) {
            return val.get<std::string>();
        }
    }

    /* Try snake_case fallback */
    std::string snake_key{field.name};
    if (j.contains(snake_key)) {
        const auto& val = j[snake_key];
        if (val.is_string()) {
            return val.get<std::string>();
        }
    }

    return std::nullopt;
}

} // namespace aw::gitsync
