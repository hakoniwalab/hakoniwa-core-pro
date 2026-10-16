#include "runtime_recovery.hpp"

#include "runtime_probe.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <regex>
#include <vector>

#include "config/hako_config.hpp"
#include "data/hako_master_data.hpp"
#include "utils/hako_config_loader.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"

namespace {

using hako::command::ProcessProbeStatus;
using hako::command::RuntimeRecoveryResult;

RuntimeRecoveryResult refusal(const std::string& reason)
{
    return {false, "REFUSED", reason, {}};
}

bool read_runtime_pids(
    const std::filesystem::path& master_path,
    std::set<pid_type> *pids,
    std::string *error)
{
    std::ifstream file(master_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return true;
    }
    const std::streamsize size = file.tellg();
    const size_t required = sizeof(hako::utils::SharedMemoryMetaDataType) +
                            sizeof(hako::data::HakoMasterDataType);
    if (size < 0 || static_cast<size_t>(size) < required) {
        *error = "master memory is too small to prove that no process remains";
        return false;
    }
    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        *error = "master memory cannot be read";
        return false;
    }
    const auto *metadata =
        reinterpret_cast<const hako::utils::SharedMemoryMetaDataType*>(bytes.data());
    if (metadata->magic != HAKO_SHM_MAGIC ||
        metadata->version != HAKO_SHM_LAYOUT_VERSION ||
        metadata->data_size < sizeof(hako::data::HakoMasterDataType)) {
        *error = "master memory layout is incompatible; process state is uncertain";
        return false;
    }
    const auto *master = reinterpret_cast<const hako::data::HakoMasterDataType*>(
        &metadata->data[0]);
    if (master->asset_num > HAKO_DATA_MAX_ASSET_NUM) {
        *error = "master memory asset count is invalid";
        return false;
    }
    if (master->master_pid > 0) {
        pids->insert(master->master_pid);
    }
    for (uint32_t i = 0; i < master->asset_num; ++i) {
        if (master->assets_ev[i].pid > 0) {
            pids->insert(master->assets_ev[i].pid);
        }
    }
    return true;
}

} // namespace

hako::command::RuntimeRecoveryResult hako::command::recover_runtime_files()
{
    HakoConfigType config;
    try {
        hako_config_load(config);
    }
    catch (const std::exception& e) {
        return refusal(e.what());
    }
    if (config.param == nullptr ||
        !config.param.contains("shm_type") ||
        config.param["shm_type"] != "mmap" ||
        !config.param.contains("core_mmap_path") ||
        !config.param["core_mmap_path"].is_string()) {
        return refusal("recovery requires a valid mmap core_mmap_path");
    }
    const std::filesystem::path root =
        config.param["core_mmap_path"].get<std::string>();
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return refusal("core_mmap_path does not exist or is not a directory");
    }

    std::set<pid_type> pids;
    std::string read_error;
    if (!read_runtime_pids(root / "mmap-0xff.bin", &pids, &read_error)) {
        return refusal(read_error);
    }
    for (pid_type pid : pids) {
        const ProcessProbeStatus status = probe_process(pid);
        if (status == ProcessProbeStatus::Alive) {
            return refusal("a Hakoniwa process is still alive: pid=" +
                           std::to_string(pid));
        }
        if (status == ProcessProbeStatus::Unknown) {
            return refusal("process liveness is uncertain: pid=" +
                           std::to_string(pid));
        }
    }

    const std::regex mmap_name("mmap-0x[0-9a-fA-F]+\\.bin");
    const std::set<std::string> lock_names = {
        "flock.bin", "pdu_init.lock", "pro_init.lock",
    };
    std::vector<std::filesystem::path> known_files;
    std::error_code directory_error;
    for (const auto& entry : std::filesystem::directory_iterator(root, directory_error)) {
        const std::string name = entry.path().filename().string();
        if (std::regex_match(name, mmap_name) || lock_names.count(name) != 0) {
            known_files.push_back(entry.path());
        }
    }
    if (directory_error) {
        return refusal("cannot enumerate core_mmap_path");
    }
    std::sort(known_files.begin(), known_files.end());

    /* Validate the complete deletion set before changing anything. */
    for (const auto& path : known_files) {
        std::error_code error;
        if (std::filesystem::is_symlink(path, error) || error ||
            !std::filesystem::is_regular_file(path, error) || error) {
            return refusal("known runtime path is not a regular file: " + path.string());
        }
    }
    RuntimeRecoveryResult result{true, "RECOVERED", "runtime files removed", {}};
    for (const auto& path : known_files) {
        std::error_code error;
        if (!std::filesystem::remove(path, error) || error) {
            return refusal("failed to remove runtime file: " + path.string());
        }
        result.removed.push_back(path.string());
    }
    return result;
}
