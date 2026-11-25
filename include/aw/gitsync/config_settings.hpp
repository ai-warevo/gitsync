#ifndef AW_GITSYNC_CONFIG_SETTINGS_HPP
#define AW_GITSYNC_CONFIG_SETTINGS_HPP

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace aw::gitsync {

/// Final configuration for the entire application (clean types, no std::optional)
struct ConfigSettings {
    std::string log_level;
    std::string remote_branch_template;
    std::map<std::string, std::string> remote_urls;
    std::string source_remote_name;
    std::string ssh_key_types;
    int max_retries;
    std::vector<std::string> include_branches;
    std::vector<std::string> exclude_branches;
    std::vector<std::string> include_remotes;
    std::vector<std::string> exclude_remotes;
    std::string update_check_url;
    std::string release_download_url_template;
};

/// Intermediate structure for partial deserialization and merging of config layers
struct PartialConfigSettings {
    std::optional<std::string> log_level;
    std::optional<std::string> remote_branch_template;
    std::optional<std::map<std::string, std::string>> remote_urls;
    std::optional<std::string> source_remote_name;
    std::optional<std::string> ssh_key_types;
    std::optional<int> max_retries;
    std::optional<std::vector<std::string>> include_branches;
    std::optional<std::vector<std::string>> exclude_branches;
    std::optional<std::vector<std::string>> include_remotes;
    std::optional<std::vector<std::string>> exclude_remotes;
    std::optional<std::string> update_check_url;
    std::optional<std::string> release_download_url_template;

    /// Merges local config settings on top of current settings.
    /// Remote URLs are merged (extended), other fields are replaced if present.
    void merge(const PartialConfigSettings& local);

    /** Converts partial config to final ConfigSettings with defaults.
        Defaults: LogLevel="info", SourceRemoteName="origin",
        RemoteBranchTemplate="%owner%/%reponame%/%branchname%", RemoteUrls={}, SshKeyTypes="",
        UpdateCheckUrl="https://raw.githubusercontent.com/ai-warevo/gitsync/refs/heads/main/VERSION",
        ReleaseDownloadUrlTemplate="https://github.com/ai-warevo/gitsync/releases/download/v{version}/{filename}" */
    [[nodiscard]] ConfigSettings into_final_settings() const;
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_CONFIG_SETTINGS_HPP
