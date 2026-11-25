cmake_minimum_required(VERSION 3.23)

include(FetchContent)
FetchContent_Declare(
    aw_cmake_modules
    GIT_REPOSITORY https://github.com/ai-warevo/template_cpp
    GIT_TAG main)
FetchContent_MakeAvailable(aw_cmake_modules)
list(APPEND CMAKE_MODULE_PATH "${aw_cmake_modules_SOURCE_DIR}/cmake")

include(AWProject)
aw_project_prepare(NAME gitsync NAMESPACE aw::)
project(${AW_PROJECT_NAME} VERSION ${AW_PROJECT_VERSION} LANGUAGES CXX)
aw_project_setup(PREFIX GITSYNC BUILD_TESTS)
aw_check_cxx23_runtime()

include(DependencyCache)

# Fetch dependencies
aw_fetch_dependency(
    NAME nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    OPTIONS
        JSON_BuildTests OFF)

aw_fetch_dependency(
    NAME spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    OPTIONS
        SPDLOG_BUILD_TESTS OFF
        SPDLOG_BUILD_EXAMPLE OFF)

# Create core library with dependencies linked
include(Library/Target)
aw_add_library(TARGET gitsync_core TYPE STATIC ALIAS aw::gitsync_core
    SOURCES
        src/gitsync.cpp
        src/config_settings.cpp
        src/config_reader.cpp
        src/config_parser_json.cpp
        src/config_parser_yaml.cpp
        src/config_parser_common.cpp
        src/cli_args.cpp
        src/filter_matcher.cpp
        src/json_output.cpp
        src/process_runner.cpp
        src/known_hosts_service.cpp
        src/git_sync_service.cpp
        src/utils.cpp
        src/self_update.cpp
    PUBLIC_INCLUDE_DIRS include
    PUBLIC_LINKS
        nlohmann_json::nlohmann_json
        spdlog::spdlog)

# Windows version resource and icon (Windows only)
if(WIN32)
    include(Windows/WinVersion)
    aw_win_version(
        OUTVAR GITSYNC_WIN_RESOURCES
        NAME "GitSync"
        VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
        VERSION_MINOR ${PROJECT_VERSION_MINOR}
        VERSION_PATCH ${PROJECT_VERSION_PATCH}
        COMPANY_NAME "AI-Warevo"
        COMPANY_COPYRIGHT "Copyright 2026 AI-Warevo"
        FILE_DESCRIPTION "Git repository synchronization tool"
        COMMENTS "GitSync v${PROJECT_VERSION}"
        ORIGINAL_FILENAME "gitsync.exe"
        INTERNAL_NAME "gitsync"
        ICON "${CMAKE_SOURCE_DIR}/product.ico"
    )
endif()

# Create executable
aw_add_executable(TARGET gitsync SOURCES src/main.cpp ${GITSYNC_WIN_RESOURCES}
    LINK_LIBRARIES aw::gitsync_core)

target_compile_definitions(gitsync PRIVATE GITSYNC_VERSION="${PROJECT_VERSION}")

# Windows code signing (Windows only)
# Certificate setup: see template_cpp/tools/create_cert.ps1
if(WIN32)
    include(Windows/CodeSign)
    aw_code_sign_target(
        TARGET gitsync
        CERT_NAME "AI-Warevo Code Signing"
    )
endif()

if(GITSYNC_WARNINGS_AS_ERRORS)
    aw_configure_compiler(TARGET gitsync STRICT_WARNINGS)
endif()

aw_setup_tests(OPTION GITSYNC_BUILD_TESTS DIRECTORY tests)
