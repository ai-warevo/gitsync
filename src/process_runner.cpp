#include "aw/gitsync/process_runner.hpp"

#include <array>
#include <cstdlib>
#include <memory>
#include <stdexcept>

#include <spdlog/spdlog.h>

// Platform-specific popen/pclose
#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

namespace aw::gitsync {

ProcessOutput ProcessRunner::run(const std::string& file_name,
                                 const std::vector<std::string>& arguments) {
    /*
      Platform differences: use _popen/_pclose on Windows, popen/pclose on Unix.
      Redirect stderr to stdout (2>&1) for unified capture.
    */
    std::string cmd_line = file_name;
    for (const auto& arg : arguments) {
        cmd_line += " ";
        if (arg.find(' ') != std::string::npos) {
            cmd_line += "\"" + arg + "\"";
        } else {
            cmd_line += arg;
        }
    }

    std::string args_str;
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0)
            args_str += ", ";
        args_str += arguments[i];
    }
    spdlog::debug("Executing command: {} with args: {}", file_name, args_str);

    std::string full_cmd = cmd_line + " 2>&1";
    std::FILE* pipe = popen(full_cmd.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("Failed to execute command: " + file_name);
    }

    std::string stdout_str;
    std::array<char, 128> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        stdout_str += buffer.data();
    }

    int status = pclose(pipe);
    bool success = (status == 0);

    /*
      Trim trailing whitespace from captured output.
    */
    while (!stdout_str.empty() && (stdout_str.back() == '\n' || stdout_str.back() == '\r' ||
                                   stdout_str.back() == ' ' || stdout_str.back() == '\t')) {
        stdout_str.pop_back();
    }

    if (!stdout_str.empty()) {
        spdlog::debug("[{}] STDOUT:\n{}", file_name, stdout_str);
    }

    return ProcessOutput{
        .stdout_str = stdout_str,
        .stderr_str = "", /* Merged with stdout above */
        .success = success,
    };
}

} // namespace aw::gitsync
