#include "aw/gitsync/git_sync_service.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace aw::gitsync {

GitSyncService::GitSyncService(std::shared_ptr<IProcessRunner> process_runner,
                               const ConfigSettings& config)
    : process_runner_(process_runner), config_(config) {}

void GitSyncService::run() {
    ensure_inside_git_repo();
    spdlog::info("Start syncing");

    auto refspec = build_refspec();
    ensure_remotes();

    auto results = push_all(refspec);
    log_summary(results);
}

void GitSyncService::ensure_remotes() {
    for (const auto& [name, url] : config_.remote_urls) {
        ensure_remote(name, url);
    }
}

std::vector<std::pair<std::string, bool>> GitSyncService::push_all(const std::string& refspec) {
    std::vector<std::pair<std::string, bool>> results;
    for (const auto& [name, url] : config_.remote_urls) {
        bool ok = true;
        try {
            push_remote(name, refspec);
        } catch (const std::exception&) {
            ok = false;
        }
        results.emplace_back(name, ok);
    }
    return results;
}

std::tuple<uint32_t, uint32_t, uint32_t>
GitSyncService::compute_summary(const std::vector<std::pair<std::string, bool>>& results) {
    uint32_t total = static_cast<uint32_t>(results.size());
    uint32_t success = 0;
    for (const auto& [name, ok] : results) {
        if (ok) {
            success++;
        }
    }
    uint32_t error = total - success;
    return std::make_tuple(success, error, total);
}

void GitSyncService::log_summary(const std::vector<std::pair<std::string, bool>>& results) {
    auto [success, error, total] = compute_summary(results);

    if (error > 0) {
        spdlog::warn("Sync complete: {}/{} succeeded, {}/{} errors", success, total, error, total);
    } else {
        spdlog::info("Sync complete: {}/{} succeeded", success, total);
    }
}

void GitSyncService::ensure_inside_git_repo() {
    try {
        auto output = process_runner_->run(
            "git", std::vector<std::string>{"rev-parse", "--is-inside-work-tree"});
        if (!output.success) {
            throw std::runtime_error("Current directory is not a git repository: " +
                                     output.stderr_str);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Process execution error: " + std::string(e.what()));
    }
}

std::string GitSyncService::try_get_remote_url(const std::string& remote_name) {
    try {
        auto output =
            process_runner_->run("git", std::vector<std::string>{"remote", "get-url", remote_name});
        if (output.success) {
            return output.stdout_str;
        }
        /*
          If git stderr mentions "No such remote", the remote doesn't exist yet;
          return empty string to signal ensure_remote() to add it.
        */
        if (output.stderr_str.find("No such remote") != std::string::npos) {
            return "";
        }
        throw std::runtime_error("Failed to get remote url: " + output.stderr_str);
    } catch (const std::exception& e) {
        throw std::runtime_error("Process run failed: " + std::string(e.what()));
    }
}

void GitSyncService::ensure_remote(const std::string& remote_name, const std::string& remote_url) {
    auto existing_url = try_get_remote_url(remote_name);

    if (existing_url.empty()) {
        spdlog::info("{} adding remote url...", remote_name);
        auto output = process_runner_->run(
            "git", std::vector<std::string>{"remote", "add", remote_name, remote_url});
        if (!output.success) {
            throw std::runtime_error(remote_name + " add error: " + output.stderr_str);
        }
        spdlog::info("{} remote url successfully added", remote_name);
    } else {
        /*
          Case-insensitive URL comparison: GitHub/GitLab normalize URLs,
          so "https://github.com/owner/repo" == "https://GITHUB.COM/owner/repo".
        */
        bool urls_match = std::equal(existing_url.begin(), existing_url.end(), remote_url.begin(),
                                     remote_url.end(), [](unsigned char c1, unsigned char c2) {
                                         return std::tolower(c1) == std::tolower(c2);
                                     });

        if (urls_match) {
            spdlog::debug("{} remote url already exists, skipping", remote_name);
        } else {
            spdlog::info("{} updating remote url...", remote_name);
            auto output = process_runner_->run(
                "git", std::vector<std::string>{"remote", "set-url", remote_name, remote_url});
            if (!output.success) {
                throw std::runtime_error(remote_name + " set-url error: " + output.stderr_str);
            }
            spdlog::info("{} remote url successfully updated", remote_name);
        }
    }
}

std::string GitSyncService::build_refspec() {
    auto source_output = process_runner_->run(
        "git", std::vector<std::string>{"remote", "get-url", config_.source_remote_name});

    if (!source_output.success) {
        throw std::runtime_error("Git remote url error: " + source_output.stderr_str);
    }
    auto source_remote_url = source_output.stdout_str;

    auto branch_output =
        process_runner_->run("git", std::vector<std::string>{"branch", "--show-current"});

    if (!branch_output.success) {
        throw std::runtime_error("Git branch error: " + branch_output.stderr_str);
    }
    std::string branch_name = branch_output.stdout_str;

    auto remote_branch =
        build_remote_branch(config_.remote_branch_template, source_remote_url, branch_name);

    return branch_name + ":" + remote_branch;
}

void GitSyncService::push_remote(const std::string& remote_name, const std::string& refspec) {
    auto output =
        process_runner_->run("git", std::vector<std::string>{"push", remote_name, refspec});

    if (!output.success) {
        spdlog::error("{} push failed: {}", remote_name, output.stderr_str);
        throw std::runtime_error(remote_name + " failed to push changes");
    }

    spdlog::info("{} push successfully completed", remote_name);
}

std::string GitSyncService::build_remote_branch(const std::string& template_str,
                                                const std::string& origin_url,
                                                const std::string& branch_name) {
    /*
      Extract repo path from origin URL (handle both SSH and HTTPS formats).
      SSH: git@github.com:owner/repo.git
      HTTPS: https://github.com/owner/repo.git
    */
    std::string without_scheme = origin_url;
    if (without_scheme.find("https://") == 0) {
        without_scheme = without_scheme.substr(8);
    } else if (without_scheme.find("http://") == 0) {
        without_scheme = without_scheme.substr(7);
    }
    std::string repo_part;
    auto colon_pos = without_scheme.find(':');
    if (colon_pos != std::string::npos) {
        repo_part = without_scheme.substr(colon_pos + 1);
    } else {
        auto slash_pos = without_scheme.find('/');
        if (slash_pos != std::string::npos) {
            repo_part = without_scheme.substr(slash_pos + 1);
        } else {
            repo_part = without_scheme;
        }
    }

    /* Strip .git suffix if present (some URLs lack it). */
    if (repo_part.length() >= 4 && repo_part.substr(repo_part.length() - 4) == ".git") {
        repo_part = repo_part.substr(0, repo_part.length() - 4);
    }
    auto slash_pos = repo_part.find('/');
    std::string owner, reponame;
    if (slash_pos != std::string::npos) {
        owner = repo_part.substr(0, slash_pos);
        reponame = repo_part.substr(slash_pos + 1);
    } else {
        owner = "";
        reponame = "";
    }

    if (owner.empty() || reponame.empty()) {
        throw std::runtime_error("Invalid origin url: " + origin_url);
    }

    /* Replace placeholders: %owner%, %reponame%, %branchname%. */
    std::string result = template_str;
    size_t pos = 0;
    while ((pos = result.find("%owner%", pos)) != std::string::npos) {
        result.replace(pos, 7, owner);
        pos += owner.length();
    }
    pos = 0;
    while ((pos = result.find("%reponame%", pos)) != std::string::npos) {
        result.replace(pos, 10, reponame);
        pos += reponame.length();
    }
    pos = 0;
    while ((pos = result.find("%branchname%", pos)) != std::string::npos) {
        result.replace(pos, 12, branch_name);
        pos += branch_name.length();
    }

    return sanitize_branch_name(result);
}

std::string GitSyncService::sanitize_branch_name(const std::string& name) {
    /*
      Split by '/', sanitize each component (allow alphanumeric, dash, underscore),
      then rejoin. This prevents git ref errors from invalid characters.
    */
    std::vector<std::string> components;
    std::string current;
    for (char c : name) {
        if (c == '/') {
            components.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    components.push_back(current);

    std::vector<std::string> sanitized;
    for (const auto& comp : components) {
        std::string safe;
        for (char c : comp) {
            if (std::isalnum(c) || c == '-' || c == '_') {
                safe += c;
            } else {
                safe += '_';
            }
        }
        sanitized.push_back(safe);
    }

    std::string result;
    for (size_t i = 0; i < sanitized.size(); ++i) {
        if (i > 0) {
            result += '/';
        }
        result += sanitized[i];
    }
    return result;
}

} // namespace aw::gitsync
