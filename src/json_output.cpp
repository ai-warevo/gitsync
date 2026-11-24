#include "aw/gitsync/json_output.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace aw::gitsync {

json JsonOutput::to_json() const {
    json j;
    j["status"] = status;
    j["exit_code"] = exit_code;
    j["dry_run"] = dry_run;
    j["timestamp"] = timestamp;
    j["duration"] = duration;

    /* Synced remotes array */
    json remotes_array = json::array();
    for (const auto& remote : synced_remotes) {
        json remote_obj;
        remote_obj["remote"] = remote.remote_name;
        remote_obj["success"] = remote.success;
        
        json branches_array = json::array();
        for (const auto& branch : remote.branches_pushed) {
            branches_array.push_back(branch);
        }
        remote_obj["branches_pushed"] = branches_array;

        json errors_array = json::array();
        for (const auto& error : remote.errors) {
            errors_array.push_back(error);
        }
        remote_obj["errors"] = errors_array;
        
        remotes_array.push_back(remote_obj);
    }
    j["synced_remotes"] = remotes_array;

    /* Warnings array */
    json warnings_array = json::array();
    for (const auto& warning : warnings) {
        warnings_array.push_back(warning);
    }
    j["warnings"] = warnings_array;

    /* Errors array */
    json errors_array = json::array();
    for (const auto& error : errors) {
        errors_array.push_back(error);
    }
    j["errors"] = errors_array;

    return j;
}

static std::string format_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

static std::string format_duration(double secs) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << secs << "s";
    return oss.str();
}

JsonOutput JsonOutput::make_success(bool dry_run,
                                  const std::vector<SyncRemoteResult>& results,
                                  double duration_secs) {
    JsonOutput output;
    output.status = "success";
    output.exit_code = 0;
    output.dry_run = dry_run;
    output.timestamp = format_timestamp();
    output.duration = format_duration(duration_secs);
    output.synced_remotes = results;
    return output;
}

JsonOutput JsonOutput::make_partial(bool dry_run,
                                  const std::vector<SyncRemoteResult>& results,
                                  double duration_secs) {
    JsonOutput output;
    output.status = "partial";
    output.exit_code = 2;
    output.dry_run = dry_run;
    output.timestamp = format_timestamp();
    output.duration = format_duration(duration_secs);
    output.synced_remotes = results;
    output.warnings.push_back("Some remotes failed to sync");
    return output;
}

JsonOutput JsonOutput::make_error(const std::string& error_msg, double duration_secs) {
    JsonOutput output;
    output.status = "error";
    output.exit_code = 1;
    output.timestamp = format_timestamp();
    output.duration = format_duration(duration_secs);
    output.errors.push_back(error_msg);
    return output;
}

} // namespace aw::gitsync
