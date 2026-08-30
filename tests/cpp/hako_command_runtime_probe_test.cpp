#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "config/hako_config.hpp"
#include "data/hako_master_data.hpp"
#include "runtime_probe.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"

namespace {

pid_type current_pid()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

class RuntimeProbeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        const char* previous_config = std::getenv("HAKO_CONFIG_PATH");
        had_previous_config_ = previous_config != nullptr;
        if (had_previous_config_) {
            previous_config_ = previous_config;
        }
        directory_ = std::filesystem::temp_directory_path() /
                     ("hako-runtime-probe-" + std::to_string(current_pid()) + "-" +
                      std::to_string(sequence_++));
        std::filesystem::create_directories(directory_);
        config_path_ = directory_ / "cpp_core_config.json";
        master_path_ = directory_ / "mmap-0xff.bin";
        std::ofstream config(config_path_);
        config << "{\n"
               << "  \"shm_type\": \"mmap\",\n"
               << "  \"core_mmap_path\": \"" << directory_.generic_string() << "\"\n"
               << "}\n";
        config.close();
#ifdef _WIN32
        _putenv_s("HAKO_CONFIG_PATH", config_path_.string().c_str());
#else
        setenv("HAKO_CONFIG_PATH", config_path_.string().c_str(), 1);
#endif
    }

    void TearDown() override
    {
#ifdef _WIN32
        _putenv_s("HAKO_CONFIG_PATH", had_previous_config_ ? previous_config_.c_str() : "");
#else
        if (had_previous_config_) {
            setenv("HAKO_CONFIG_PATH", previous_config_.c_str(), 1);
        }
        else {
            unsetenv("HAKO_CONFIG_PATH");
        }
#endif
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    void write_master(pid_type pid, uint32_t magic = HAKO_SHM_MAGIC)
    {
        const size_t file_size = sizeof(hako::utils::SharedMemoryMetaDataType) +
                                 sizeof(hako::data::HakoMasterDataType);
        std::vector<unsigned char> bytes(file_size, 0);
        auto* metadata = reinterpret_cast<hako::utils::SharedMemoryMetaDataType*>(bytes.data());
        metadata->magic = magic;
        metadata->version = HAKO_SHM_LAYOUT_VERSION;
        metadata->shm_id = -1;
        metadata->sem_id = HAKO_SHARED_MEMORY_ID_0;
        metadata->data_size = sizeof(hako::data::HakoMasterDataType);
        auto* master = reinterpret_cast<hako::data::HakoMasterDataType*>(&metadata->data[0]);
        master->master_pid = pid;
        std::ofstream file(master_path_, std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

    static inline unsigned int sequence_ = 0;
    std::filesystem::path directory_;
    std::filesystem::path config_path_;
    std::filesystem::path master_path_;
    bool had_previous_config_ = false;
    std::string previous_config_;
};

} // namespace

TEST_F(RuntimeProbeTest, ReportsMissingMasterMemoryWithoutTakingALock)
{
    auto result = hako::command::probe_runtime();
    EXPECT_EQ(result.status, hako::command::RuntimeProbeStatus::MasterMemoryNotFound);
}

TEST_F(RuntimeProbeTest, AcceptsCompatibleMemoryOwnedByLiveMasterPid)
{
    write_master(current_pid());
    auto result = hako::command::probe_runtime();
    EXPECT_EQ(result.status, hako::command::RuntimeProbeStatus::Running);
    EXPECT_EQ(result.master_pid, current_pid());
}

TEST_F(RuntimeProbeTest, RejectsStaleMasterPid)
{
    write_master((std::numeric_limits<pid_type>::max)());
    auto result = hako::command::probe_runtime();
    EXPECT_EQ(result.status, hako::command::RuntimeProbeStatus::MasterNotRunning);
}

TEST_F(RuntimeProbeTest, RejectsIncompatibleMasterMemory)
{
    write_master(current_pid(), 0);
    auto result = hako::command::probe_runtime();
    EXPECT_EQ(result.status, hako::command::RuntimeProbeStatus::InvalidMasterMemory);
}
