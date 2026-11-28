#ifndef AW_GITSYNC_JSON_OUTPUT_HPP
#define AW_GITSYNC_JSON_OUTPUT_HPP

#include <chrono>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace aw::gitsync {

using json = nlohmann::json;

/// Sync result for a single remote
struct SyncRemoteResult {
    std::string remote_name;
    std::vector<std::string> branches_pushed;
    std::vector<std::string> errors;
    bool success = false;
};

/// Complete sync output in JSON format
struct JsonOutput {
    std::string status; // "success", "partial", "error"
    int exit_code = 0;
    bool dry_run = false;
    std::string timestamp;
    std::string duration; // "1.23s"

    std::vector<SyncRemoteResult> synced_remotes;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    /// Convert to JSON representation
    json to_json() const;

    /// Create from components
    static JsonOutput make_success(bool dry_run, const std::vector<SyncRemoteResult>& results,
                                   double duration_secs);

    static JsonOutput make_partial(bool dry_run, const std::vector<SyncRemoteResult>& results,
                                   double duration_secs);

    static JsonOutput make_error(const std::string& error_msg, double duration_secs);
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_JSON_OUTPUT_HPP
