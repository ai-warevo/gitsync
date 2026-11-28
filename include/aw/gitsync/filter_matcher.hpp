#ifndef AW_GITSYNC_FILTER_MATCHER_HPP
#define AW_GITSYNC_FILTER_MATCHER_HPP

#include <string>
#include <string_view>
#include <vector>

namespace aw::gitsync {

/// Pattern matching for filter rules (glob-like patterns)
class FilterMatcher {
public:
    /// Check if name matches any include pattern
    /// If include_patterns is empty, matches everything
    static bool matches_include(const std::string& name,
                                const std::vector<std::string>& include_patterns);

    /// Check if name should be excluded
    static bool matches_exclude(const std::string& name,
                                const std::vector<std::string>& exclude_patterns);

    /// Check if name passes both include and exclude filters
    static bool passes_filters(const std::string& name,
                               const std::vector<std::string>& include_patterns,
                               const std::vector<std::string>& exclude_patterns);

private:
    /// Simple glob pattern matching (* and ? support)
    static bool glob_match(std::string_view pattern, std::string_view name);
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_FILTER_MATCHER_HPP
