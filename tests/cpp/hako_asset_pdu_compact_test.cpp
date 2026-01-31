#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "hako.hpp"
#include "hako_asset.h"
#include "hako_conductor.h"
#include "hako_asset_pdu.hpp"

namespace {

std::filesystem::path test_base_dir(const std::string& suffix)
{
    auto base_dir = std::filesystem::temp_directory_path() / ("hako_pdu_test_" + suffix);
    std::filesystem::create_directories(base_dir);
    return base_dir;
}

void cleanup_test_dirs()
{
    const char* suffixes[] = {"compact", "legacy", "core"};
    for (const auto* suffix : suffixes) {
        std::error_code ec;
        std::filesystem::remove_all(test_base_dir(suffix), ec);
    }
}

std::filesystem::path write_compact_test_files()
{
    auto base_dir = test_base_dir("compact");

    auto pdutypes_path = base_dir / "pdutypes.json";
    std::ofstream pdutypes(pdutypes_path);
    pdutypes
        << "[\n"
        << "  { \"channel_id\": 0, \"pdu_size\": 72, \"name\": \"pos\", \"type\": \"geometry_msgs/Twist\" },\n"
        << "  { \"channel_id\": 1, \"pdu_size\": 56, \"name\": \"status\", \"type\": \"hako_msgs/DroneStatus\" }\n"
        << "]\n";
    pdutypes.close();

    auto pdudef_path = base_dir / "pdudef.json";
    std::ofstream pdudef(pdudef_path);
    pdudef
        << "{\n"
        << "  \"paths\": [ { \"id\": \"default\", \"path\": \"pdutypes.json\" } ],\n"
        << "  \"robots\": [ { \"name\": \"Robot1\", \"pdutypes_id\": \"default\" } ]\n"
        << "}\n";
    pdudef.close();

    return pdudef_path;
}

std::filesystem::path write_core_config()
{
    auto base_dir = test_base_dir("core");
    auto mmap_dir = base_dir / "mmap";
    std::filesystem::create_directories(mmap_dir);

    auto config_path = base_dir / "cpp_core_config.json";
    std::ofstream config(config_path);
    config
        << "{\n"
        << "  \"shm_type\": \"mmap\",\n"
        << "  \"core_mmap_path\": \"" << mmap_dir.string() << "\"\n"
        << "}\n";
    config.close();

    return config_path;
}

std::filesystem::path write_legacy_test_files()
{
    auto base_dir = test_base_dir("legacy");
    auto pdudef_path = base_dir / "pdudef.json";
    std::ofstream pdudef(pdudef_path);
    pdudef
        << "{\n"
        << "  \"robots\": [\n"
        << "    {\n"
        << "      \"name\": \"Robot1\",\n"
        << "      \"shm_pdu_readers\": [\n"
        << "        {\n"
        << "          \"type\": \"geometry_msgs/Twist\",\n"
        << "          \"org_name\": \"pos\",\n"
        << "          \"name\": \"Robot1_pos\",\n"
        << "          \"channel_id\": 0,\n"
        << "          \"pdu_size\": 72,\n"
        << "          \"write_cycle\": 1,\n"
        << "          \"method_type\": \"SHM\"\n"
        << "        },\n"
        << "        {\n"
        << "          \"type\": \"hako_msgs/DroneStatus\",\n"
        << "          \"org_name\": \"status\",\n"
        << "          \"name\": \"Robot1_status\",\n"
        << "          \"channel_id\": 1,\n"
        << "          \"pdu_size\": 56,\n"
        << "          \"write_cycle\": 1,\n"
        << "          \"method_type\": \"SHM\"\n"
        << "        }\n"
        << "      ],\n"
        << "      \"shm_pdu_writers\": []\n"
        << "    }\n"
        << "  ]\n"
        << "}\n";
    pdudef.close();
    return pdudef_path;
}

const hako::asset::PduWriter* find_writer(const hako::asset::Robot& robot, const std::string& org_name)
{
    for (const auto& writer : robot.pdu_writers) {
        if (writer.org_name == org_name) {
            return &writer;
        }
    }
    return nullptr;
}

const hako::asset::PduReader* find_reader(const hako::asset::Robot& robot, const std::string& org_name)
{
    for (const auto& reader : robot.pdu_readers) {
        if (reader.org_name == org_name) {
            return &reader;
        }
    }
    return nullptr;
}

}

TEST(HakoAssetPduConfigTest, CompactConfigBuildsLegacyView)
{
    std::cerr << "TEST: init" << std::endl;
    auto core_config = write_core_config();
    setenv("HAKO_CONFIG_PATH", core_config.string().c_str(), 1);

    std::cerr << "TEST: write_test_files" << std::endl;
    auto config_path = write_compact_test_files();

    std::cerr << "TEST: conductor_start" << std::endl;
    hako_conductor_start(10000, 10000);

    std::cerr << "TEST: hako_asset_register" << std::endl;
    hako_asset_callbacks_t callbacks{};
    callbacks.on_initialize = [](hako_asset_context_t*) { return 0; };
    callbacks.on_manual_timing_control = nullptr;
    callbacks.on_simulation_step = [](hako_asset_context_t*) { return 0; };
    callbacks.on_reset = [](hako_asset_context_t*) { return 0; };
    int ret = hako_asset_register(
        "TestAsset",
        config_path.string().c_str(),
        &callbacks,
        1000,
        HAKO_ASSET_MODEL_CONTROLLER
    );
    ASSERT_EQ(ret, 0);

    std::cerr << "TEST: get_pdus_compact" << std::endl;
    std::vector<hako::asset::RobotCompact> compact;
    ASSERT_TRUE(hako::asset::hako_asset_get_pdus_compact(compact));
    ASSERT_EQ(compact.size(), 1u);
    EXPECT_EQ(compact[0].name, "Robot1");
    ASSERT_EQ(compact[0].pdus.size(), 2u);
    EXPECT_EQ(compact[0].pdus[0].name, "pos");
    EXPECT_EQ(compact[0].pdus[1].name, "status");

    std::cerr << "TEST: get_pdus_legacy" << std::endl;
    std::vector<hako::asset::Robot> legacy;
    ASSERT_TRUE(hako::asset::hako_asset_get_pdus(legacy));
    ASSERT_EQ(legacy.size(), 1u);
    const auto& robot = legacy[0];
    EXPECT_EQ(robot.name, "Robot1");

    auto pos_writer = find_writer(robot, "pos");
    ASSERT_NE(pos_writer, nullptr);
    EXPECT_EQ(pos_writer->name, "Robot1_pos");
    EXPECT_EQ(pos_writer->channel_id, 0);
    EXPECT_EQ(pos_writer->pdu_size, 72);
    EXPECT_EQ(pos_writer->write_cycle, 1);
    EXPECT_EQ(pos_writer->method_type, "SHM");

    auto status_reader = find_reader(robot, "status");
    ASSERT_NE(status_reader, nullptr);
    EXPECT_EQ(status_reader->name, "Robot1_status");
    EXPECT_EQ(status_reader->channel_id, 1);
    EXPECT_EQ(status_reader->pdu_size, 56);
    EXPECT_EQ(status_reader->write_cycle, 1);

    std::cerr << "TEST: conductor_stop" << std::endl;
    hako_conductor_stop();

    std::cerr << "TEST: destroy" << std::endl;
    hako::destroy();
    std::cerr << "TEST: done" << std::endl;
    cleanup_test_dirs();
}

TEST(HakoAssetPduConfigTest, LegacyConfigBuildsCompactView)
{
    std::cerr << "TEST: init" << std::endl;
    auto core_config = write_core_config();
    setenv("HAKO_CONFIG_PATH", core_config.string().c_str(), 1);

    std::cerr << "TEST: write_test_files" << std::endl;
    auto config_path = write_legacy_test_files();

    std::cerr << "TEST: conductor_start" << std::endl;
    hako_conductor_start(10000, 10000);

    std::cerr << "TEST: hako_asset_register" << std::endl;
    hako_asset_callbacks_t callbacks{};
    callbacks.on_initialize = [](hako_asset_context_t*) { return 0; };
    callbacks.on_manual_timing_control = nullptr;
    callbacks.on_simulation_step = [](hako_asset_context_t*) { return 0; };
    callbacks.on_reset = [](hako_asset_context_t*) { return 0; };
    int ret = hako_asset_register(
        "TestAsset",
        config_path.string().c_str(),
        &callbacks,
        1000,
        HAKO_ASSET_MODEL_CONTROLLER
    );
    ASSERT_EQ(ret, 0);

    std::cerr << "TEST: get_pdus_compact" << std::endl;
    std::vector<hako::asset::RobotCompact> compact;
    ASSERT_TRUE(hako::asset::hako_asset_get_pdus_compact(compact));
    ASSERT_EQ(compact.size(), 1u);
    EXPECT_EQ(compact[0].name, "Robot1");
    ASSERT_EQ(compact[0].pdus.size(), 2u);
    EXPECT_EQ(compact[0].pdus[0].name, "pos");
    EXPECT_EQ(compact[0].pdus[1].name, "status");

    std::cerr << "TEST: get_pdus_legacy" << std::endl;
    std::vector<hako::asset::Robot> legacy;
    ASSERT_TRUE(hako::asset::hako_asset_get_pdus(legacy));
    ASSERT_EQ(legacy.size(), 1u);
    const auto& robot = legacy[0];
    EXPECT_EQ(robot.name, "Robot1");

    auto pos_writer = find_writer(robot, "pos");
    ASSERT_NE(pos_writer, nullptr);
    EXPECT_EQ(pos_writer->name, "Robot1_pos");
    EXPECT_EQ(pos_writer->channel_id, 0);
    EXPECT_EQ(pos_writer->pdu_size, 72);
    EXPECT_EQ(pos_writer->write_cycle, 1);
    EXPECT_EQ(pos_writer->method_type, "SHM");

    auto status_reader = find_reader(robot, "status");
    ASSERT_NE(status_reader, nullptr);
    EXPECT_EQ(status_reader->name, "Robot1_status");
    EXPECT_EQ(status_reader->channel_id, 1);
    EXPECT_EQ(status_reader->pdu_size, 56);
    EXPECT_EQ(status_reader->write_cycle, 1);

    std::cerr << "TEST: conductor_stop" << std::endl;
    hako_conductor_stop();

    std::cerr << "TEST: destroy" << std::endl;
    hako::destroy();
    std::cerr << "TEST: done" << std::endl;
    cleanup_test_dirs();
}
