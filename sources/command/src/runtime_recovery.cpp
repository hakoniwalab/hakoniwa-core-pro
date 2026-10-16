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
#include "utils/hako_share/hako_master_lock.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"

namespace {

using hako::command::ProcessProbeStatus;
using hako::command::RuntimeRecoveryResult;

RuntimeRecoveryResult refusal(const std::string& reason)
{
    return {false, "REFUSED", reason, {}, {}, {}};
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
    const size_t header = sizeof(hako::utils::SharedMemoryMetaDataType);
    const size_t required = header + sizeof(hako::data::HakoMasterDataType);
    if (size < 0) {
        *error = "master memory cannot be read";
        return false;
    }
    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (size > 0 && !file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        *error = "master memory cannot be read";
        return false;
    }
    const auto *metadata =
        static_cast<size_t>(size) >= header
            ? reinterpret_cast<const hako::utils::SharedMemoryMetaDataType*>(bytes.data())
            : nullptr;
    // Master memory written by an older Core: another layout version, or no
    // header at all (before the layout header existed) with at least the old
    // master data size. Its process table cannot be read, and recovery is an
    // operator action that requires every process to be stopped (mixed Core
    // versions are unsupported), so it is removed like a legacy flock.bin.
    // A current-layout file that is truncated or inconsistent stays a refusal.
    const bool other_layout =
        metadata != nullptr && metadata->magic == HAKO_SHM_MAGIC &&
        metadata->version != HAKO_SHM_LAYOUT_VERSION;
    const bool no_header =
        (metadata == nullptr || metadata->magic != HAKO_SHM_MAGIC) &&
        static_cast<size_t>(size) >= sizeof(hako::data::HakoMasterDataType);
    if (other_layout || no_header) {
        return true;
    }
    if (static_cast<size_t>(size) < required ||
        metadata->data_size < sizeof(hako::data::HakoMasterDataType)) {
        *error = "master memory is too small or inconsistent to prove that no process remains";
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

hako::command::RuntimeRecoveryResult
hako::command::detail::remove_known_runtime_files(
    const std::vector<std::filesystem::path>& paths,
    const RemoveRuntimeFile& remove_file)
{
    std::vector<std::filesystem::path> ordered = paths;
    std::stable_sort(
        ordered.begin(), ordered.end(),
        [](const auto& left, const auto& right) {
            const bool left_flock = left.filename() == "flock.bin";
            const bool right_flock = right.filename() == "flock.bin";
            if (left_flock != right_flock) {
                return !left_flock;
            }
            return left.filename().string() < right.filename().string();
        });
    RuntimeRecoveryResult result{
        true, "RECOVERED", "runtime files removed", {}, {}, {}};
    for (size_t index = 0; index < ordered.size(); ++index) {
        const auto& path = ordered[index];
        std::error_code error;
        if (!remove_file(path, error) || error) {
            result.recovered = false;
            result.status = "PARTIAL";
            result.reason = "failed to remove one or more runtime files";
            for (size_t remaining = index; remaining < ordered.size(); ++remaining) {
                result.remaining.push_back(ordered[remaining].string());
            }
            result.failures.push_back({
                path.string(),
                error ? error.message() : "remove operation returned false",
            });
            return result;
        }
        result.removed.push_back(path.string());
    }
    return result;
}

hako::command::RuntimeRecoveryResult hako::command::detail::recover_runtime_files(
    const RemoveRuntimeFile& remove_file)
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
    std::error_code root_error;
    if (std::filesystem::is_symlink(root, root_error) || root_error ||
        !std::filesystem::exists(root, root_error) || root_error ||
        !std::filesystem::is_directory(root, root_error) || root_error) {
        return refusal("core_mmap_path does not exist or is not a directory");
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
    /* Validate the complete deletion set before reading or changing anything. */
    for (const auto& path : known_files) {
        std::error_code error;
        if (std::filesystem::is_symlink(path, error) || error ||
            !std::filesystem::is_regular_file(path, error) || error) {
            return refusal("known runtime path is not a regular file: " + path.string());
        }
    }

    const std::filesystem::path flock_path = root / "flock.bin";
    if (std::filesystem::exists(flock_path)) {
        using namespace hako::utils::sem::flock;
        const MasterLockInspection lock = inspect_master_lock(0, false);
        if (lock.health == MasterLockHealth::Unknown &&
            (lock.unknown_cause == MasterLockUnknownCause::Identity ||
             lock.unknown_cause == MasterLockUnknownCause::Contention ||
             lock.unknown_cause == MasterLockUnknownCause::System)) {
            return refusal(
                "master lock ownership cannot be verified: " + lock.reason);
        }
        if (lock.health == MasterLockHealth::Ok &&
            (lock.state != MasterLockState::Unlocked || lock.owner_pid != 0 ||
             lock.owner_start_token != 0)) {
            return refusal(
                "master lock is owned by a live process: pid=" +
                std::to_string(lock.owner_pid));
        }
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

    return detail::remove_known_runtime_files(known_files, remove_file);
}

hako::command::RuntimeRecoveryResult hako::command::recover_runtime_files()
{
    return detail::recover_runtime_files(
        [](const std::filesystem::path& path, std::error_code& error) {
            return std::filesystem::remove(path, error);
        });
}
