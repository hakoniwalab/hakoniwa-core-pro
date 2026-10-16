#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <vector>

namespace hako::command {

struct RuntimeRecoveryFailure {
    std::string path;
    std::string reason;
};

struct RuntimeRecoveryResult {
    bool recovered = false;
    std::string status;
    std::string reason;
    std::vector<std::string> removed;
    std::vector<std::string> remaining;
    std::vector<RuntimeRecoveryFailure> failures;
};

RuntimeRecoveryResult recover_runtime_files();

namespace detail {
using RemoveRuntimeFile = std::function<bool(
    const std::filesystem::path&, std::error_code&)>;

RuntimeRecoveryResult remove_known_runtime_files(
    const std::vector<std::filesystem::path>& paths,
    const RemoveRuntimeFile& remove_file);
RuntimeRecoveryResult recover_runtime_files(const RemoveRuntimeFile& remove_file);
} // namespace detail

} // namespace hako::command
