#ifndef AW_GITSYNC_SELF_UPDATE_HPP
#define AW_GITSYNC_SELF_UPDATE_HPP

#include <filesystem>
#include <optional>
#include <string>

namespace aw::gitsync {

struct UpdateError {
    std::string msg;
};

/*
  Get the path to the currently running executable.
  Cross-platform: Windows (GetModuleFileNameW), macOS (_NSGetExecutablePath), Linux (/proc/self/exe).
  Returns: (path, optional error).
*/
std::pair<std::filesystem::path, std::optional<UpdateError>> get_self_path();

/*
  Perform self-update by replacing the running executable with a staged binary.
  Process: current → .old, update_file → current, restart with --cleanup flag.
  Rolls back if copy fails.
  Returns: (success, optional error).
*/
std::pair<bool, std::optional<UpdateError>> self_update(
    const std::filesystem::path& current_exe,
    std::string_view update_filename);

/*
  Clean up old binary (.old file) after restart.
  Retries up to 20 times with 100ms delays.
*/
void cleanup_old_binary(const std::filesystem::path& current_exe);

}  // namespace aw::gitsync

#endif  // AW_GITSYNC_SELF_UPDATE_HPP
