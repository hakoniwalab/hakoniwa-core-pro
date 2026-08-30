#include "runtime_probe.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <sstream>

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#endif

#include "config/hako_config.hpp"
#include "data/hako_master_data.hpp"
#include "utils/hako_config_loader.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"

namespace {

bool is_process_alive(pid_type pid)
{
    if (pid <= 0) {
        return false;
    }
#ifdef WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process == nullptr) {
        return GetLastError() == ERROR_ACCESS_DENIED;
    }
    DWORD exit_code = 0;
    const bool alive = GetExitCodeProcess(process, &exit_code) && (exit_code == STILL_ACTIVE);
    CloseHandle(process);
    return alive;
#else
    if (kill(pid, 0) == 0) {
        return true;
    }
    return errno == EPERM;
#endif
}

hako::command::RuntimeProbeResult make_result(
    hako::command::RuntimeProbeStatus status,
    const std::string& path,
    pid_type pid,
    const std::string& message)
{
    return {status, pid, path, message};
}

} // namespace

hako::command::RuntimeProbeResult hako::command::probe_runtime()
{
    HakoConfigType config;
    try {
        hako_config_load(config);
    }
    catch (const std::exception& e) {
        return make_result(RuntimeProbeStatus::SystemError, "", 0, e.what());
    }
    if (config.param == nullptr) {
        return make_result(
            RuntimeProbeStatus::ConfigNotFound, "", 0,
            "Hakoniwa core configuration was not found");
    }
    if (!config.param.contains("shm_type") || config.param["shm_type"] != "mmap") {
        return make_result(
            RuntimeProbeStatus::UnsupportedSharedMemory, "", 0,
            "hako-cmd bounded lock mode requires shm_type=mmap");
    }
    if (!config.param.contains("core_mmap_path") ||
        !config.param["core_mmap_path"].is_string()) {
        return make_result(
            RuntimeProbeStatus::ConfigNotFound, "", 0,
            "core_mmap_path is missing from Hakoniwa core configuration");
    }

    std::ostringstream path_builder;
    path_builder << config.param["core_mmap_path"].get<std::string>()
                 << "/mmap-0x" << std::hex << HAKO_SHARED_MEMORY_ID_0 << ".bin";
    const std::string path = path_builder.str();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return make_result(
            RuntimeProbeStatus::MasterMemoryNotFound, path, 0,
            "Hakoniwa master memory was not found");
    }
    const std::streamsize file_size = file.tellg();
    if ((file_size < 0) ||
        (static_cast<uint64_t>(file_size) < sizeof(hako::utils::SharedMemoryMetaDataType))) {
        return make_result(
            RuntimeProbeStatus::InvalidMasterMemory, path, 0,
            "Hakoniwa master memory is smaller than expected");
    }

    hako::utils::SharedMemoryMetaDataType metadata{};
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(&metadata), sizeof(metadata))) {
        return make_result(
            RuntimeProbeStatus::SystemError, path, 0,
            "Failed to read Hakoniwa master memory metadata");
    }
    if ((metadata.magic != HAKO_SHM_MAGIC) ||
        (metadata.version != HAKO_SHM_LAYOUT_VERSION)) {
        return make_result(
            RuntimeProbeStatus::InvalidMasterMemory, path, 0,
            "Hakoniwa master memory metadata is incompatible");
    }

    const uint64_t minimum_data_size =
        offsetof(hako::data::HakoMasterDataType, master_pid) + sizeof(pid_type);
    const uint64_t expected_file_size =
        sizeof(hako::utils::SharedMemoryMetaDataType) + metadata.data_size;
    if ((metadata.data_size < minimum_data_size) ||
        (static_cast<uint64_t>(file_size) < expected_file_size)) {
        return make_result(
            RuntimeProbeStatus::InvalidMasterMemory, path, 0,
            "Hakoniwa master memory size metadata is invalid");
    }

    pid_type master_pid = 0;
    const std::streamoff pid_offset =
        static_cast<std::streamoff>(offsetof(hako::utils::SharedMemoryMetaDataType, data)) +
        static_cast<std::streamoff>(offsetof(hako::data::HakoMasterDataType, master_pid));
    file.seekg(pid_offset, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(&master_pid), sizeof(master_pid))) {
        return make_result(
            RuntimeProbeStatus::SystemError, path, 0,
            "Failed to read Hakoniwa master PID");
    }
    if (!is_process_alive(master_pid)) {
        std::ostringstream message;
        message << "Hakoniwa master is not running (stale pid=" << master_pid << ")";
        return make_result(
            RuntimeProbeStatus::MasterNotRunning, path, master_pid, message.str());
    }
    return make_result(
        RuntimeProbeStatus::Running, path, master_pid,
        "Hakoniwa master is running");
}
