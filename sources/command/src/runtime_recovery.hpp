#pragma once

#include <string>
#include <vector>

namespace hako::command {

struct RuntimeRecoveryResult {
    bool recovered = false;
    std::string status;
    std::string reason;
    std::vector<std::string> removed;
};

RuntimeRecoveryResult recover_runtime_files();

} // namespace hako::command
