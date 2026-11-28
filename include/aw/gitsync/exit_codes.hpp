#ifndef AW_GITSYNC_EXIT_CODES_HPP
#define AW_GITSYNC_EXIT_CODES_HPP

namespace aw::gitsync {

/// Standard exit codes for CI/CD integration
enum class ExitCode {
    SUCCESS = 0,          ///< All remotes synced successfully
    GENERAL_ERROR = 1,    ///< Unhandled exception or unexpected error
    PARTIAL_SUCCESS = 2,  ///< Some remotes synced, some failed (warnings logged)
    CONFIG_ERROR = 3,     ///< Invalid or missing configuration
    GIT_ERROR = 4,        ///< Git operation error (not in repo, branch mismatch, etc)
    PERMISSION_ERROR = 5, ///< SSH key denied or file permission denied
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_EXIT_CODES_HPP
