#ifndef AW_GITSYNC_UTILS_HPP
#define AW_GITSYNC_UTILS_HPP

#include <string>
#include <string_view>

namespace aw::gitsync {

/// Parse the latest version from a GitHub API JSON response.
/// Returns the version string from the response, or falls back to current_version if parsing fails.
std::string parse_latest_version(std::string_view json_response, std::string_view current_version);

/// Compare two semantic versions (X.Y.Z format).
/// Returns: 1 if v1 > v2, -1 if v1 < v2, 0 if equal, -2 if parsing error
int compare_versions(std::string_view v1, std::string_view v2);

}  // namespace aw::gitsync

#endif // AW_GITSYNC_UTILS_HPP
