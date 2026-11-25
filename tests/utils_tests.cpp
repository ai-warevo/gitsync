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
