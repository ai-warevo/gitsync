#ifndef AW_GITSYNC_GIT_SYNC_SERVICE_HPP
#define AW_GITSYNC_GIT_SYNC_SERVICE_HPP

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "aw/gitsync/config_settings.hpp"
#include "aw/gitsync/process_runner.hpp"

namespace aw::gitsync {

/// Service for syncing git repositories to multiple remote mirrors
class GitSyncService {
public:
    GitSyncService(std::shared_ptr<IProcessRunner> process_runner, const ConfigSettings& config);

    /// Main sync loop: ensures remotes, builds refspec, and pushes to all remotes
    void run();

    /// Push to all configured remotes with the given refspec.
    /// Returns list of (remote_name, success) pairs.
    std::vector<std::pair<std::string, bool>> push_all(const std::string& refspec);

    /// Compute summary statistics from push results
    std::tuple<uint32_t, uint32_t, uint32_t>
    compute_summary(const std::vector<std::pair<std::string, bool>>& results);

    // Public static helpers for testing
    static std::string build_remote_branch(const std::string& template_str,
                                           const std::string& origin_url,
                                           const std::string& branch_name);
    static std::string sanitize_branch_name(const std::string& name);

private:
    std::shared_ptr<IProcessRunner> process_runner_;
    ConfigSettings config_;

    /// Ensure we're inside a git repository
    void ensure_inside_git_repo();

    /// Ensure all configured remotes exist and are up-to-date
    void ensure_remotes();

    /// Ensure a single remote exists with the correct URL
    void ensure_remote(const std::string& remote_name, const std::string& remote_url);

    /// Try to get the URL of an existing remote (returns empty optional if not found)
    std::string try_get_remote_url(const std::string& remote_name);

    /// Build the refspec from current branch and source remote
    std::string build_refspec();

    /// Push changes to a single remote
    void push_remote(const std::string& remote_name, const std::string& refspec);

    /// Log summary of push results
    void log_summary(const std::vector<std::pair<std::string, bool>>& results);
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_GIT_SYNC_SERVICE_HPP
