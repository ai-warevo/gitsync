#include "aw/gitsync/self_update.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

#include "aw/gitsync/utils.hpp"

#ifdef _WIN32
#    include <windows.h>
#elif defined(__APPLE__)
#    include <limits.h>
#    include <mach-o/dyld.h>
#    include <unistd.h>
#else
#    include <limits.h>
#    include <unistd.h>
#endif

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace aw::gitsync {

std::pair<fs::path, std::optional<UpdateError>> get_self_path() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) {
        return {fs::path(), UpdateError{"GetModuleFileNameW failed"}};
    }
    buffer.resize(len);
    return {fs::path(std::move(buffer)), std::nullopt};
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        return {fs::path(), UpdateError{"_NSGetExecutablePath failed"}};
    }
    std::error_code ec;
    auto path = fs::canonical(buffer, ec);
    if (ec)
        return {fs::path(), UpdateError{ec.message()}};
    return {path, std::nullopt};
#else
    std::error_code ec;
    auto path = fs::read_symlink("/proc/self/exe", ec);
    if (ec)
        return {fs::path(), UpdateError{ec.message()}};
    return {path, std::nullopt};
#endif
}

void cleanup_old_binary(const fs::path& current_exe) {
    fs::path old_exe = current_exe;
    old_exe += ".old";

    for (int i = 0; i < 20; ++i) {
        std::error_code ec;
        if (!fs::exists(old_exe, ec) || fs::remove(old_exe, ec)) {
            break;
        }
        std::this_thread::sleep_for(100ms);
    }

    /* Also remove update file if it exists */
    fs::path update_exe = current_exe.parent_path() / get_update_file_name();

    for (int i = 0; i < 20; ++i) {
        std::error_code ec;
        if (!fs::exists(update_exe, ec) || fs::remove(update_exe, ec)) {
            return;
        }
        std::this_thread::sleep_for(100ms);
    }
}

std::pair<bool, std::optional<UpdateError>> self_update(const fs::path& current_exe,
                                                        std::string_view update_filename) {
    fs::path update_bin = current_exe.parent_path() / update_filename;
    fs::path old_exe = current_exe;
    old_exe += ".old";

    std::error_code ec;
    if (!fs::exists(update_bin, ec)) {
        return {false, UpdateError{std::format("Update file not found: {}", update_bin.string())}};
    }

    /* 1. Remove old .old if it exists */
    fs::remove(old_exe, ec);

    /* 2. Rename current binary to .old */
    fs::rename(current_exe, old_exe, ec);
    if (ec) {
        return {false, UpdateError{std::format("Failed to rename current file: {}", ec.message())}};
    }

    /* 3. Copy new file in place of old */
    fs::copy_file(update_bin, current_exe, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        fs::rename(old_exe, current_exe, ec); /* rollback */
        return {false, UpdateError{std::format("Failed to copy update: {}", ec.message())}};
    }

#ifndef _WIN32
    /* 4. Set execute permissions for Unix */
    fs::permissions(current_exe,
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add, ec);
#endif

    /* 5. Restart with cleanup flag */
#ifdef _WIN32
    STARTUPINFOW si{.cb = sizeof(si)};
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + current_exe.wstring() + L"\" --cleanup";

    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si,
                        &pi)) {
        return {false, UpdateError{std::format("CreateProcessW error: {}", GetLastError())}};
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return {true, std::nullopt};
#else
    std::string exe_str = current_exe.string();
    std::string arg_cleanup = "--cleanup";
    char* args[] = {exe_str.data(), arg_cleanup.data(), nullptr};

    execv(exe_str.c_str(), args);
    return {false, UpdateError{"execv error on restart"}};
#endif
}

} // namespace aw::gitsync
