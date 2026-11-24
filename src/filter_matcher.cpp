#include "aw/gitsync/filter_matcher.hpp"

namespace aw::gitsync {

bool FilterMatcher::glob_match(std::string_view pattern, std::string_view name) {
    size_t p = 0, n = 0;

    while (p < pattern.size() && n < name.size()) {
        if (pattern[p] == '*') {
            if (p + 1 >= pattern.size()) {
                /* Trailing * matches rest of string */
                return true;
            }
            /* Recursively match rest of pattern at each position in name */
            for (size_t i = n; i <= name.size(); ++i) {
                if (glob_match(pattern.substr(p + 1), name.substr(i))) {
                    return true;
                }
            }
            return false;
        } else if (pattern[p] == '?') {
            /* ? matches exactly one character */
            ++p;
            ++n;
        } else if (pattern[p] == name[n]) {
            ++p;
            ++n;
        } else {
            return false;
        }
    }

    /* Consume any trailing * in pattern */
    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }

    return p == pattern.size() && n == name.size();
}

bool FilterMatcher::matches_include(const std::string& name,
                                   const std::vector<std::string>& include_patterns) {
    if (include_patterns.empty()) {
        /* Empty include list includes everything (no filtering) */
        return true;
    }

    for (const auto& pattern : include_patterns) {
        if (glob_match(pattern, name)) {
            return true;
        }
    }
    return false;
}

bool FilterMatcher::matches_exclude(const std::string& name,
                                   const std::vector<std::string>& exclude_patterns) {
    for (const auto& pattern : exclude_patterns) {
        if (glob_match(pattern, name)) {
            return true;
        }
    }
    return false;
}

bool FilterMatcher::passes_filters(const std::string& name,
                                  const std::vector<std::string>& include_patterns,
                                  const std::vector<std::string>& exclude_patterns) {
    return matches_include(name, include_patterns) && !matches_exclude(name, exclude_patterns);
}

} // namespace aw::gitsync
