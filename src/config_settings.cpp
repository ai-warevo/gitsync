#include "aw/gitsync/config_settings.hpp"

namespace aw::gitsync {

void PartialConfigSettings::merge(const PartialConfigSettings& local) {
    if (local.log_level) {
        log_level = local.log_level;
    }
    if (local.remote_branch_template) {
        remote_branch_template = local.remote_branch_template;
    }
    if (local.source_remote_name) {
        source_remote_name = local.source_remote_name;
    }
    if (local.ssh_key_types) {
        ssh_key_types = local.ssh_key_types;
    }
    if (local.max_retries) {
        max_retries = local.max_retries;
    }
    if (local.include_branches) {
        include_branches = local.include_branches;
    }
    if (local.exclude_branches) {
        exclude_branches = local.exclude_branches;
    }
    if (local.include_remotes) {
        include_remotes = local.include_remotes;
    }
    if (local.exclude_remotes) {
        exclude_remotes = local.exclude_remotes;
    }
    if (local.update_check_url) {
        update_check_url = local.update_check_url;
    }
    if (local.release_download_url_template) {
        release_download_url_template = local.release_download_url_template;
    }
    if (local.remote_urls) {
        if (remote_urls) {
            /* Merge new entries into existing map (extends rather than replaces) */
            for (const auto& [key, value] : *local.remote_urls) {
                (*remote_urls)[key] = value;
            }
        } else {
            remote_urls = local.remote_urls;
        }
    }
}

ConfigSettings PartialConfigSettings::into_final_settings() const {
    return ConfigSettings{
        .log_level = log_level.value_or("info"),
        .remote_branch_template =
            remote_branch_template.value_or("%owner%/%reponame%/%branchname%"),
        .remote_urls = remote_urls.value_or(std::map<std::string, std::string>{}),
        .source_remote_name = source_remote_name.value_or("origin"),
        .ssh_key_types = ssh_key_types.value_or(""),
        .max_retries = max_retries.value_or(1),
        .include_branches = include_branches.value_or(std::vector<std::string>{"*"}),
        .exclude_branches = exclude_branches.value_or(std::vector<std::string>{}),
        .include_remotes = include_remotes.value_or(std::vector<std::string>{}),
        .exclude_remotes = exclude_remotes.value_or(std::vector<std::string>{"upstream", "base"}),
        .update_check_url = update_check_url.value_or(
            "https://raw.githubusercontent.com/ai-warevo/gitsync/refs/heads/main/VERSION"),
        .release_download_url_template = release_download_url_template.value_or(
            "https://github.com/ai-warevo/gitsync/releases/download/v{version}/{filename}"),
    };
}

} // namespace aw::gitsync
