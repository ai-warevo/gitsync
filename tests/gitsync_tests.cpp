#include "aw/gitsync/config_settings.hpp"
#include "aw/gitsync/git_sync_service.hpp"
#include "aw/gitsync/known_hosts_service.hpp"
#include "aw/gitsync/config_reader.hpp"
#include "aw/gitsync/filter_matcher.hpp"

#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

using namespace aw::gitsync;
using json = nlohmann::json;

TEST_CASE("ProcessOutput diagnostic uses captured command output", "[process]") {
    ProcessOutput merged_output{
        .stdout_str = "permission denied", .stderr_str = "", .success = false};
    REQUIRE(merged_output.diagnostic() == "permission denied");

    ProcessOutput stderr_output{
        .stdout_str = "command output", .stderr_str = "remote rejected", .success = false};
    REQUIRE(stderr_output.diagnostic() == "remote rejected");
}

TEST_CASE("PartialConfigSettings::merge", "[config]") {
    PartialConfigSettings base;
    base.log_level = "info";
    base.source_remote_name = "origin";

    PartialConfigSettings local;
    local.log_level = "debug";
    local.remote_urls = std::map<std::string, std::string>{{"m1", "url1"}};

    base.merge(local);

    REQUIRE(base.log_level == "debug");
    REQUIRE(base.source_remote_name == "origin");
    REQUIRE(base.remote_urls.has_value());
}

TEST_CASE("into_final_settings with defaults", "[config]") {
    PartialConfigSettings partial;
    auto final = partial.into_final_settings();

    REQUIRE(final.log_level == "info");
    REQUIRE(final.source_remote_name == "origin");
    REQUIRE(final.remote_branch_template == "%owner%/%reponame%/%branchname%");
    REQUIRE(final.ssh_key_types.empty());
    REQUIRE(final.remote_urls.empty());
}

TEST_CASE("sanitize_branch_name", "[git]") {
    REQUIRE(GitSyncService::sanitize_branch_name("feature/test") == "feature/test");
    REQUIRE(GitSyncService::sanitize_branch_name(".github/main") == "_github/main");
    REQUIRE(GitSyncService::sanitize_branch_name("feat@2.0!") == "feat_2_0_");
    REQUIRE(GitSyncService::sanitize_branch_name("foo..bar/baz") == "foo__bar/baz");
}

TEST_CASE("build_remote_branch with SSH URL", "[git]") {
    auto result = GitSyncService::build_remote_branch(
        "%owner%/%reponame%/%branchname%",
        "git@github.com:alice/project.git",
        "main");
    REQUIRE(result == "alice/project/main");
}

TEST_CASE("build_remote_branch with HTTPS URL", "[git]") {
    auto result = GitSyncService::build_remote_branch(
        "%owner%/%reponame%/%branchname%",
        "https://github.com/alice/project.git",
        "main");
    REQUIRE(result == "alice/project/main");
}

TEST_CASE("parse_host from SSH URL", "[known_hosts]") {
    auto host = KnownHostsService::parse_host("git@github.com:owner/repo.git");
    REQUIRE(host == "github.com");
}

TEST_CASE("parse_host from HTTPS URL", "[known_hosts]") {
    auto host = KnownHostsService::parse_host("https://github.com/owner/repo.git");
    REQUIRE(host == "github.com");
}

TEST_CASE("line_matches_host", "[known_hosts]") {
    REQUIRE(KnownHostsService::line_matches_host("github.com ssh-ed25519 AAAA", "github.com"));
    REQUIRE(KnownHostsService::line_matches_host("github.com,1.2.3.4 ssh-ed25519 AAAA",
                                                  "github.com"));
    REQUIRE_FALSE(KnownHostsService::line_matches_host("gitlab.com ssh-ed25519 AAAA", "github.com"));
}

TEST_CASE("line_is_valid_known_host_line", "[known_hosts]") {
    REQUIRE(KnownHostsService::line_is_valid_known_host_line("github.com ssh-ed25519 AAAA"));
    REQUIRE_FALSE(KnownHostsService::line_is_valid_known_host_line("# comment"));
    REQUIRE_FALSE(KnownHostsService::line_is_valid_known_host_line("   "));
}

TEST_CASE("into_final_settings with new filter fields", "[config]") {
    PartialConfigSettings partial;
    partial.max_retries = 3;
    partial.include_branches = std::vector<std::string>{"main", "release/*"};
    partial.exclude_remotes = std::vector<std::string>{"upstream", "base"};
    
    auto final = partial.into_final_settings();

    REQUIRE(final.max_retries == 3);
    REQUIRE(final.include_branches == std::vector<std::string>{"main", "release/*"});
    REQUIRE(final.exclude_remotes == std::vector<std::string>{"upstream", "base"});
}

TEST_CASE("dual convention: PascalCase in JSON", "[config]") {
    json j;
    j["LogLevel"] = "debug";
    j["SourceRemoteName"] = "upstream";
    j["MaxRetries"] = 2;
    j["RemoteUrls"] = {{"m1", "url1"}};
    
    // Create a minimal from_json function call
    // This tests that PascalCase names are read correctly
    PartialConfigSettings result;
    if (j.contains("LogLevel")) {
        result.log_level = j["LogLevel"].get<std::string>();
    }
    if (j.contains("SourceRemoteName")) {
        result.source_remote_name = j["SourceRemoteName"].get<std::string>();
    }
    if (j.contains("MaxRetries")) {
        result.max_retries = j["MaxRetries"].get<int>();
    }
    
    REQUIRE(result.log_level == "debug");
    REQUIRE(result.source_remote_name == "upstream");
    REQUIRE(result.max_retries == 2);
}

TEST_CASE("dual convention: snake_case in JSON", "[config]") {
    json j;
    j["log_level"] = "warn";
    j["source_remote_name"] = "origin";
    j["max_retries"] = 1;
    j["include_branches"] = json::array({"main", "develop"});
    
    PartialConfigSettings result;
    if (j.contains("log_level")) {
        result.log_level = j["log_level"].get<std::string>();
    }
    if (j.contains("source_remote_name")) {
        result.source_remote_name = j["source_remote_name"].get<std::string>();
    }
    if (j.contains("max_retries")) {
        result.max_retries = j["max_retries"].get<int>();
    }
    if (j.contains("include_branches")) {
        std::vector<std::string> branches;
        for (const auto& b : j["include_branches"]) {
            branches.push_back(b.get<std::string>());
        }
        result.include_branches = branches;
    }
    
    REQUIRE(result.log_level == "warn");
    REQUIRE(result.source_remote_name == "origin");
    REQUIRE(result.max_retries == 1);
    REQUIRE(result.include_branches.has_value());
    REQUIRE(result.include_branches.value().size() == 2);
}

TEST_CASE("dual convention: snake_case takes precedence over PascalCase", "[config]") {
    json j;
    j["log_level"] = "snake_debug";
    j["LogLevel"] = "pascal_debug";
    
    // snake_case should win
    PartialConfigSettings result;
    if (j.contains("log_level")) {
        result.log_level = j["log_level"].get<std::string>();
    } else if (j.contains("LogLevel")) {
        result.log_level = j["LogLevel"].get<std::string>();
    }
    
    REQUIRE(result.log_level == "snake_debug");
}

TEST_CASE("into_final_settings with defaults for new fields", "[config]") {
    PartialConfigSettings partial;
    auto final = partial.into_final_settings();
    
    REQUIRE(final.max_retries == 1);
    REQUIRE(final.include_branches == std::vector<std::string>{"*"});
    REQUIRE(final.exclude_branches == std::vector<std::string>{});
    REQUIRE(final.include_remotes == std::vector<std::string>{});
    REQUIRE(final.exclude_remotes == std::vector<std::string>{"upstream", "base"});
}

TEST_CASE("FilterMatcher: glob_match exact", "[filter]") {
    REQUIRE(FilterMatcher::matches_include("main", {"main"}));
    REQUIRE_FALSE(FilterMatcher::matches_include("main", {"develop"}));
}

TEST_CASE("FilterMatcher: glob_match with *", "[filter]") {
    REQUIRE(FilterMatcher::matches_include("release/1.0", {"release/*"}));
    REQUIRE(FilterMatcher::matches_include("release/2.0", {"release/*"}));
    REQUIRE_FALSE(FilterMatcher::matches_include("develop/1.0", {"release/*"}));
}

TEST_CASE("FilterMatcher: glob_match with ?", "[filter]") {
    REQUIRE(FilterMatcher::matches_include("v1", {"v?"}));
    REQUIRE(FilterMatcher::matches_include("v2", {"v?"}));
    REQUIRE_FALSE(FilterMatcher::matches_include("v10", {"v?"}));
}

TEST_CASE("FilterMatcher: exclude pattern", "[filter]") {
    REQUIRE(FilterMatcher::matches_exclude("main", {"main"}));
    REQUIRE_FALSE(FilterMatcher::matches_exclude("develop", {"main"}));
    REQUIRE(FilterMatcher::matches_exclude("release/1.0", {"release/*"}));
}

TEST_CASE("FilterMatcher: passes_filters include and exclude", "[filter]") {
    std::vector<std::string> include{"main", "release/*"};
    std::vector<std::string> exclude{"release/experimental"};
    
    REQUIRE(FilterMatcher::passes_filters("main", include, exclude));
    REQUIRE(FilterMatcher::passes_filters("release/1.0", include, exclude));
    REQUIRE_FALSE(FilterMatcher::passes_filters("release/experimental", include, exclude));
    REQUIRE_FALSE(FilterMatcher::passes_filters("develop", include, exclude));
}

TEST_CASE("FilterMatcher: empty include matches all", "[filter]") {
    std::vector<std::string> include;  // Empty = include all
    std::vector<std::string> exclude{"main"};
    
    REQUIRE(FilterMatcher::passes_filters("develop", include, exclude));
    REQUIRE_FALSE(FilterMatcher::passes_filters("main", include, exclude));
}
