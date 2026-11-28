#ifndef AW_GITSYNC_CLI_ARGS_HPP
#define AW_GITSYNC_CLI_ARGS_HPP

#include <optional>
#include <string>
#include <vector>

namespace aw::gitsync {

/// Parsed command-line arguments
struct CliArgs {
    bool dry_run = false;
    bool verbose = false;
    bool quiet = false;
    bool json_output = false;
    bool help = false;
    bool version = false;
    bool update = false;
    bool skip_host = false;
    bool cleanup = false; /* Internal: cleanup after self-update */
    int jobs = 1;         // Default: sequential (1 job)

    /// Override log level if CLI flags set
    std::optional<std::string> log_level_override() const {
        if (verbose) {
            return "debug";
        }
        if (quiet) {
            return "error";
        }
        return std::nullopt;
    }
};

/// Parse command-line arguments from argv
/// Returns parsed CliArgs or throws on invalid arguments
CliArgs parse_cli_args(const std::vector<std::string>& args);

} // namespace aw::gitsync

#endif // AW_GITSYNC_CLI_ARGS_HPP
