#ifndef AW_GITSYNC_KNOWN_HOSTS_SERVICE_HPP
#define AW_GITSYNC_KNOWN_HOSTS_SERVICE_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "aw/gitsync/config_settings.hpp"
#include "aw/gitsync/process_runner.hpp"

namespace aw::gitsync {

/// Service for warming up SSH known_hosts by scanning remote hosts
class KnownHostsService {
public:
    KnownHostsService(std::shared_ptr<IProcessRunner> process_runner, const ConfigSettings& config);

    /// Warm up known_hosts at the default location (~/.ssh/known_hosts)
    void warm_up();

    /// Warm up known_hosts at a specific path
    void warm_up_at(const std::filesystem::path& known_hosts_path);

    // Public static helpers for testing
    static bool line_is_valid_known_host_line(const std::string& line);
    static bool line_matches_host(const std::string& line, const std::string& host);
    static std::string parse_host(const std::string& remote_url);

private:
    std::shared_ptr<IProcessRunner> process_runner_;
    ConfigSettings config_;

    /// Extract hosts from remote URLs and deduplicate
    std::vector<std::string> get_hosts();

    /// Get the default known_hosts path
    static std::filesystem::path get_known_hosts_path();

    /// Load existing known_hosts file
    static std::vector<std::string> load_known_hosts(const std::filesystem::path& known_hosts_path);

    /// Build updated known_hosts by preserving non-matching lines and adding scanned ones
    std::vector<std::string> build_updated_known_hosts(const std::vector<std::string>& existing,
                                                       const std::vector<std::string>& hosts);

    /// Run ssh-keyscan on all hosts and collect output
    std::vector<std::string> scan_hosts(const std::vector<std::string>& hosts);

    /// Run ssh-keyscan with optional fallback if first attempt is empty
    ProcessOutput execute_scan_with_fallback(const std::string& host);

    /// Parse ssh-keyscan output and extract valid lines for the host
    static std::vector<std::string> parse_scan_output(const ProcessOutput& output,
                                                      const std::string& host);

    /// Save known_hosts file
    static void save_known_hosts(const std::filesystem::path& known_hosts_path,
                                 const std::vector<std::string>& lines);
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_KNOWN_HOSTS_SERVICE_HPP
