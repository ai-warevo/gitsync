#ifndef AW_GITSYNC_CONFIG_SCHEMA_HPP
#define AW_GITSYNC_CONFIG_SCHEMA_HPP

#include <array>
#include <string_view>

namespace aw::gitsync {

/** Configuration field descriptor: name, JSON/YAML key, default value */
struct ConfigField {
    std::string_view name;          /* snake_case field name */
    std::string_view json_key;      /* PascalCase key for JSON/YAML */
    std::string_view default_value; /* default if not provided */
};

/* Complete schema of all config fields */
constexpr std::array CONFIG_SCHEMA{
    ConfigField{"log_level", "LogLevel", "info"},
    ConfigField{"remote_branch_template", "RemoteBranchTemplate",
                "%owner%/%reponame%/%branchname%"},
    ConfigField{"source_remote_name", "SourceRemoteName", "origin"},
    ConfigField{"ssh_key_types", "SshKeyTypes", ""},
    ConfigField{"update_check_url", "UpdateCheckUrl",
                "https://raw.githubusercontent.com/ai-warevo/gitsync/refs/heads/main/VERSION"},
    ConfigField{"release_download_url_template", "ReleaseDownloadUrlTemplate",
                "https://github.com/ai-warevo/gitsync/releases/download/v{version}/{filename}"},
    ConfigField{"max_retries", "MaxRetries", "1"},
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_CONFIG_SCHEMA_HPP
