#include "aw/gitsync/utils.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace aw::gitsync;

TEST_CASE("parse_latest_version extracts version from JSON", "[utils]") {
    const std::string json = R"({
        "tag_name": "v1.2.3",
        "name": "Release v1.2.3"
    })";

    auto result = parse_latest_version(json, "0.0.0");
    REQUIRE(result == "v1.2.3");
}

TEST_CASE("parse_latest_version returns current_version on failure", "[utils]") {
    const std::string json = R"({ "name": "Release" })";  // Missing tag_name

    auto result = parse_latest_version(json, "1.0.0");
    REQUIRE(result == "1.0.0");
}

TEST_CASE("parse_latest_version handles plain-text VERSION file", "[utils]") {
    const std::string text = "2.0.20\n";

    auto result = parse_latest_version(text, "1.0.0");
    REQUIRE(result == "2.0.20");
}

TEST_CASE("parse_latest_version handles plain-text with v prefix", "[utils]") {
    const std::string text = "  v3.1.0  \n";

    auto result = parse_latest_version(text, "1.0.0");
    REQUIRE(result == "v3.1.0");
}

TEST_CASE("compare_versions handles equal versions", "[utils]") {
    REQUIRE(compare_versions("1.2.3", "1.2.3") == 0);
    REQUIRE(compare_versions("v1.2.3", "1.2.3") == 0);  // With/without 'v' prefix
}

TEST_CASE("compare_versions detects newer version", "[utils]") {
    REQUIRE(compare_versions("1.2.4", "1.2.3") == 1);
    REQUIRE(compare_versions("2.0.0", "1.9.9") == 1);
    REQUIRE(compare_versions("1.3.0", "1.2.9") == 1);
}

TEST_CASE("compare_versions detects older version", "[utils]") {
    REQUIRE(compare_versions("1.2.2", "1.2.3") == -1);
    REQUIRE(compare_versions("0.9.0", "1.0.0") == -1);
    REQUIRE(compare_versions("1.2.9", "1.3.0") == -1);
}

TEST_CASE("compare_versions handles different length versions", "[utils]") {
    REQUIRE(compare_versions("1.2.3.4", "1.2.3") == 1);  // Longer is greater
    REQUIRE(compare_versions("1.2", "1.2.0") == -1);     // Shorter is less
}

TEST_CASE("compare_versions handles malformed versions", "[utils]") {
    REQUIRE(compare_versions("abc", "1.2.3") == -2);  // Parse error
    REQUIRE(compare_versions("1.2.x", "1.2.3") == -2);  // Parse error
}

TEST_CASE("find_asset_url extracts download URL from GitHub release JSON", "[utils]") {
    const std::string json = R"({
        "assets": [
            {
                "name": "gitsync.exe",
                "browser_download_url": "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/gitsync.exe"
            }
        ]
    })";

    auto result = find_asset_url(json, "gitsync.exe");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/gitsync.exe");
}

TEST_CASE("find_asset_url returns nullopt when asset not found", "[utils]") {
    const std::string json = R"({
        "assets": [
            {
                "name": "gitsync",
                "browser_download_url": "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/gitsync"
            }
        ]
    })";

    auto result = find_asset_url(json, "gitsync.exe");
    REQUIRE(!result.has_value());
}

TEST_CASE("find_asset_url returns nullopt when assets key missing", "[utils]") {
    const std::string json = R"({
        "tag_name": "v2.0.26"
    })";

    auto result = find_asset_url(json, "gitsync.exe");
    REQUIRE(!result.has_value());
}

TEST_CASE("find_asset_url returns nullopt when browser_download_url missing", "[utils]") {
    const std::string json = R"({
        "assets": [
            {
                "name": "gitsync.exe"
            }
        ]
    })";

    auto result = find_asset_url(json, "gitsync.exe");
    REQUIRE(!result.has_value());
}

TEST_CASE("find_asset_url handles multiple assets", "[utils]") {
    const std::string json = R"({
        "assets": [
            {
                "name": "gitsync",
                "browser_download_url": "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/gitsync"
            },
            {
                "name": "gitsync.exe",
                "browser_download_url": "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/gitsync.exe"
            },
            {
                "name": "SHA256SUMS.txt",
                "browser_download_url": "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/SHA256SUMS.txt"
            }
        ]
    })";

    auto result = find_asset_url(json, "gitsync.exe");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "https://github.com/ai-warevo/gitsync/releases/download/v2.0.26/gitsync.exe");
}

TEST_CASE("find_asset_url handles invalid JSON", "[utils]") {
    const std::string json = R"({ broken json )";

    auto result = find_asset_url(json, "gitsync.exe");
    REQUIRE(!result.has_value());
}

TEST_CASE("find_asset_url extracts Linux binary", "[utils]") {
    const std::string json = R"({
        "assets": [
            {
                "name": "gitsync",
                "browser_download_url": "https://github.com/ai-warevo/gitsync/releases/download/v2.0.28/gitsync"
            }
        ]
    })";

    auto result = find_asset_url(json, "gitsync");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "https://github.com/ai-warevo/gitsync/releases/download/v2.0.28/gitsync");
}
