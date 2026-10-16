#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
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
#include "runtime_recovery.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"
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

class RuntimeRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        const char *previous = std::getenv("HAKO_CONFIG_PATH");
        had_previous_ = previous != nullptr;
        if (had_previous_) previous_ = previous;
        root_ = std::filesystem::temp_directory_path() /
                ("hako-runtime-recovery-" + std::to_string(current_pid()));
        std::filesystem::create_directories(root_);
        config_ = root_ / "cpp_core_config.json";
        std::ofstream output(config_);
        output << "{\"shm_type\":\"mmap\",\"core_mmap_path\":\""
               << root_.generic_string() << "\"}";
        output.close();
#ifdef _WIN32
        _putenv_s("HAKO_CONFIG_PATH", config_.string().c_str());
#else
        setenv("HAKO_CONFIG_PATH", config_.string().c_str(), 1);
#endif
    }

    void TearDown() override
    {
#ifdef _WIN32
        _putenv_s("HAKO_CONFIG_PATH", had_previous_ ? previous_.c_str() : "");
#else
        if (had_previous_) setenv("HAKO_CONFIG_PATH", previous_.c_str(), 1);
        else unsetenv("HAKO_CONFIG_PATH");
#endif
        std::error_code error;
        std::filesystem::remove_all(root_, error);
    }

    void write_master(pid_type pid)
    {
        const size_t size = sizeof(hako::utils::SharedMemoryMetaDataType) +
                            sizeof(hako::data::HakoMasterDataType);
        std::vector<unsigned char> bytes(size, 0);
        auto *metadata = reinterpret_cast<hako::utils::SharedMemoryMetaDataType*>(
            bytes.data());
        metadata->magic = HAKO_SHM_MAGIC;
        metadata->version = HAKO_SHM_LAYOUT_VERSION;
        metadata->data_size = sizeof(hako::data::HakoMasterDataType);
        auto *master = reinterpret_cast<hako::data::HakoMasterDataType*>(
            &metadata->data[0]);
        master->master_pid = pid;
        std::ofstream output(root_ / "mmap-0xff.bin", std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

    void create_unlocked_flock()
    {
        ASSERT_GE(hako::utils::sem::flock::create(0x00FF), 0);
        hako::utils::sem::flock::destroy(0x00FF);
    }

    std::filesystem::path root_;
    std::filesystem::path config_;
    bool had_previous_ = false;
    std::string previous_;
};

} // namespace

TEST_F(RuntimeRecoveryTest, RefusesWhileKnownProcessIsAlive)
{
    write_master(current_pid());
    std::ofstream(root_ / "flock.bin") << "runtime";
    const auto result = hako::command::recover_runtime_files();
    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_TRUE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
    EXPECT_TRUE(std::filesystem::exists(root_ / "flock.bin"));
}

TEST_F(RuntimeRecoveryTest, RefusesUncertainMasterLayout)
{
    std::ofstream(root_ / "mmap-0xff.bin") << "corrupt";
    const auto result = hako::command::recover_runtime_files();
    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_TRUE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
}

TEST_F(RuntimeRecoveryTest, RefusesLiveFlockOwnerWithoutMasterMemory)
{
    ASSERT_GE(hako::utils::sem::flock::create(0x00FF), 0);
    hako::utils::sem::flock::master_lock(0x00FF);
    const auto result = hako::command::recover_runtime_files();
    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_NE(result.reason.find("live process"), std::string::npos);
    EXPECT_TRUE(std::filesystem::exists(root_ / "flock.bin"));
    hako::utils::sem::flock::master_unlock(0x00FF);
    hako::utils::sem::flock::destroy(0x00FF);
}

TEST_F(RuntimeRecoveryTest, RemovesAllKnownPatternsButPreservesUnrelatedFiles)
{
    write_master(0);
    for (const char *name : {
             "mmap-0x100.bin", "mmap-0xabcdef.bin",
             "pdu_init.lock", "pro_init.lock"}) {
        std::ofstream(root_ / name) << "runtime";
    }
    create_unlocked_flock();
    std::ofstream(root_ / "operator-notes.txt") << "keep";
    const auto result = hako::command::recover_runtime_files();
    ASSERT_TRUE(result.recovered) << result.reason;
    EXPECT_EQ(result.removed.size(), 6U);
    EXPECT_TRUE(std::filesystem::exists(root_ / "operator-notes.txt"));
}

TEST(RuntimeRecoveryRemovalTest, PreservesProgressWhenRemovalPartiallyFails)
{
    const std::vector<std::filesystem::path> paths = {
        "first.bin", "second.bin", "third.bin",
    };
    const auto result = hako::command::detail::remove_known_runtime_files(
        paths,
        [](const std::filesystem::path& path, std::error_code& error) {
            if (path == "second.bin") {
                error = std::make_error_code(std::errc::permission_denied);
                return false;
            }
            return true;
        });
    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "PARTIAL");
    ASSERT_EQ(result.removed.size(), 1U);
    EXPECT_EQ(result.removed[0], "first.bin");
    ASSERT_EQ(result.remaining.size(), 2U);
    EXPECT_EQ(result.remaining[0], "second.bin");
    EXPECT_EQ(result.remaining[1], "third.bin");
    ASSERT_EQ(result.failures.size(), 1U);
    EXPECT_EQ(result.failures[0].path, "second.bin");
    EXPECT_FALSE(result.failures[0].reason.empty());
}
