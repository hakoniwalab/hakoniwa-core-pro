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
#include "utils/hako_share/hako_shared_memory_factory.hpp"

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

    void write_master(pid_type master_pid, pid_type asset_pid = 0)
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
        master->master_pid = master_pid;
        if (asset_pid > 0) {
            master->asset_num = 1;
            master->assets_ev[0].pid = asset_pid;
        }
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

TEST_F(RuntimeRecoveryTest, RefusesWhileRecordedAssetIsAlive)
{
    write_master(0, current_pid());
    const auto result = hako::command::recover_runtime_files();
    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_NE(result.reason.find("still alive"), std::string::npos);
    EXPECT_TRUE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
}

TEST_F(RuntimeRecoveryTest, RefusesUncertainMasterLayout)
{
    std::ofstream(root_ / "mmap-0xff.bin") << "corrupt";
    const auto result = hako::command::recover_runtime_files();
    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_TRUE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
}

TEST_F(RuntimeRecoveryTest, RemovesMasterMemoryOfAnotherLayoutVersion)
{
    // An older Core's master memory (another layout version): its process
    // table cannot be read; recovery assumes every process is stopped.
    std::vector<char> bytes(sizeof(hako::utils::SharedMemoryMetaDataType) +
                            sizeof(hako::data::HakoMasterDataType), 0);
    auto *metadata =
        reinterpret_cast<hako::utils::SharedMemoryMetaDataType*>(bytes.data());
    metadata->magic = HAKO_SHM_MAGIC;
    metadata->version = HAKO_SHM_LAYOUT_VERSION + 1;
    std::ofstream(root_ / "mmap-0xff.bin", std::ios::binary)
        .write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    create_unlocked_flock();
    const auto result = hako::command::recover_runtime_files();
    ASSERT_TRUE(result.recovered) << result.reason;
    EXPECT_FALSE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
}

TEST_F(RuntimeRecoveryTest, RemovesHeaderlessMasterMemoryOfAnOlderCore)
{
    // Before the layout header: the master data alone, no magic.
    std::vector<char> bytes(sizeof(hako::data::HakoMasterDataType), 0x5a);
    std::ofstream(root_ / "mmap-0xff.bin", std::ios::binary)
        .write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    const auto result = hako::command::recover_runtime_files();
    ASSERT_TRUE(result.recovered) << result.reason;
    EXPECT_FALSE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
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
    ASSERT_FALSE(result.removed.empty());
    EXPECT_EQ(std::filesystem::path(result.removed.back()).filename(), "flock.bin");
}

TEST(RuntimeRecoveryRemovalTest, PreservesProgressWhenRemovalPartiallyFails)
{
    const std::vector<std::filesystem::path> paths = {
        "flock.bin", "first.bin", "second.bin", "third.bin",
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
    ASSERT_EQ(result.remaining.size(), 3U);
    EXPECT_EQ(result.remaining[0], "second.bin");
    EXPECT_EQ(result.remaining[1], "third.bin");
    EXPECT_EQ(result.remaining[2], "flock.bin");
    ASSERT_EQ(result.failures.size(), 1U);
    EXPECT_EQ(result.failures[0].path, "second.bin");
    EXPECT_FALSE(result.failures[0].reason.empty());
}

TEST_F(RuntimeRecoveryTest, RemovesLegacyRuntime)
{
    write_master(0);
    std::ofstream legacy(root_ / "flock.bin", std::ios::binary);
    std::vector<unsigned char> counters(4096, 0);
    legacy.write(
        reinterpret_cast<const char*>(counters.data()), counters.size());
    legacy.close();

    const auto result = hako::command::recover_runtime_files();

    EXPECT_TRUE(result.recovered) << result.reason;
    EXPECT_EQ(result.status, "RECOVERED");
    EXPECT_FALSE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
    EXPECT_FALSE(std::filesystem::exists(root_ / "flock.bin"));
}

TEST_F(RuntimeRecoveryTest, EmptyRuntimeDirectoryIsAlreadyRecovered)
{
    const auto result = hako::command::recover_runtime_files();

    EXPECT_TRUE(result.recovered) << result.reason;
    EXPECT_EQ(result.status, "RECOVERED");
    EXPECT_TRUE(result.removed.empty());
}

TEST_F(RuntimeRecoveryTest, FreshSessionCanStartAfterSafeRecovery)
{
    write_master(0);
    create_unlocked_flock();
    const auto result = hako::command::recover_runtime_files();
    ASSERT_TRUE(result.recovered) << result.reason;

    auto memory = hako::utils::hako_shared_memory_create("mmap");
    ASSERT_NE(memory, nullptr);
    EXPECT_EQ(memory->create_memory(HAKO_SHARED_MEMORY_ID_0, 128), 0);
}

TEST_F(RuntimeRecoveryTest, RefusesKnownRuntimeSymlinkWithoutTouchingTarget)
{
    write_master(0);
    create_unlocked_flock();
    const auto target = root_ / "operator-notes.txt";
    const auto link = root_ / "mmap-0x100.bin";
    std::ofstream(target) << "keep";
    std::error_code error;
    std::filesystem::create_symlink(target, link, error);
    if (error) {
        GTEST_SKIP() << "symlink creation unavailable: " << error.message();
    }

    const auto result = hako::command::recover_runtime_files();

    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_NE(result.reason.find("not a regular file"), std::string::npos);
    EXPECT_TRUE(std::filesystem::exists(target));
    EXPECT_EQ(std::filesystem::file_size(target), 4U);
    EXPECT_TRUE(std::filesystem::exists(root_ / "mmap-0xff.bin"));
    EXPECT_TRUE(std::filesystem::exists(root_ / "flock.bin"));
}

TEST_F(RuntimeRecoveryTest, RefusesSymlinkedRuntimeRoot)
{
    const auto real_root =
        root_.parent_path() / (root_.filename().string() + "-real");
    std::error_code error;
    std::filesystem::rename(root_, real_root, error);
    ASSERT_FALSE(error) << error.message();
    std::filesystem::create_directory_symlink(real_root, root_, error);
    if (error) {
        std::filesystem::rename(real_root, root_);
        GTEST_SKIP() << "directory symlink creation unavailable: "
                     << error.message();
    }

    const auto result = hako::command::recover_runtime_files();

    EXPECT_FALSE(result.recovered);
    EXPECT_EQ(result.status, "REFUSED");
    EXPECT_NE(result.reason.find("does not exist or is not a directory"),
              std::string::npos);

    std::filesystem::remove(root_, error);
    ASSERT_FALSE(error) << error.message();
    std::filesystem::rename(real_root, root_, error);
    ASSERT_FALSE(error) << error.message();
}
