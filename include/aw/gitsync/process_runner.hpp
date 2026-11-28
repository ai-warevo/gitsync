#ifndef AW_GITSYNC_PROCESS_RUNNER_HPP
#define AW_GITSYNC_PROCESS_RUNNER_HPP

#include <string>
#include <vector>

namespace aw::gitsync {

/// Output from running a process
struct ProcessOutput {
    std::string stdout_str;
    std::string stderr_str;
    bool success{false};

    /// Returns stderr, or merged command output when stderr was redirected.
    [[nodiscard]] std::string diagnostic() const {
        return stderr_str.empty() ? stdout_str : stderr_str;
    }
};

/// Interface for running external commands
class IProcessRunner {
public:
    virtual ~IProcessRunner() = default;

    /// Runs an external command with the given arguments and returns the output.
    /// Throws std::runtime_error if the process cannot be executed.
    virtual ProcessOutput run(const std::string& file_name,
                              const std::vector<std::string>& arguments) = 0;
};

/// Implementation of IProcessRunner that executes real commands
class ProcessRunner : public IProcessRunner {
public:
    ProcessOutput run(const std::string& file_name,
                      const std::vector<std::string>& arguments) override;
};

} // namespace aw::gitsync

#endif // AW_GITSYNC_PROCESS_RUNNER_HPP
