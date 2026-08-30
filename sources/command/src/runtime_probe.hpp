#pragma once

#include <string>
#include "types/hako_types.hpp"

namespace hako::command {

enum class RuntimeProbeStatus {
    Running,
    ConfigNotFound,
    UnsupportedSharedMemory,
    MasterMemoryNotFound,
    InvalidMasterMemory,
    MasterNotRunning,
    SystemError,
};

struct RuntimeProbeResult {
    RuntimeProbeStatus status;
    pid_type master_pid;
    std::string master_memory_path;
    std::string message;

    bool is_running() const
    {
        return status == RuntimeProbeStatus::Running;
    }
};

RuntimeProbeResult probe_runtime();

} // namespace hako::command
