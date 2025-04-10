#include "dump_meta.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <sys/stat.h>
#include <nlohmann/json.hpp>
#include "utils/hako_config_loader.hpp"

using json = nlohmann::ordered_json;
void hako::command::HakoMetaDumper::parse() 
{
    HakoConfigType config;
    hako_config_load(config);
    if (config.param == nullptr) {
        throw std::runtime_error("Failed to load configuration file.");
    }
    if (config.param["shm_type"] != "mmap") {
        throw std::runtime_error("shm_type is not mmap.");
    }
    if (config.param["core_mmap_path"].is_null()) {
        throw std::runtime_error("core_mmap_path is null.");
    }
    std::string core_mmap_path = config.param["core_mmap_path"];
    size_t total_size = sizeof(hako::data::HakoMasterDataType) + sizeof(hako::utils::SharedMemoryMetaDataType);
    
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s/mmap-0x%x.bin", core_mmap_path.c_str(), HAKO_SHARED_MEMORY_ID_0);
    std::string filepath(buf);

    struct stat stat_buf;
    if (stat(filepath.c_str(), &stat_buf) != 0) {
        throw std::runtime_error("File not found: " + filepath);
    }

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (static_cast<size_t>(size) < total_size) {
        throw std::runtime_error("File size is smaller than expected MasterData size.");
    }

    buffer_.resize(size);
    if (!file.read(reinterpret_cast<char*>(buffer_.data()), size)) {
        throw std::runtime_error("Failed to read from file: " + filepath);
    }

    file.close();
    shm_ptr_ = reinterpret_cast<hako::utils::SharedMemoryMetaDataType*>(buffer_.data());
    master_data_ptr_ = reinterpret_cast<hako::data::HakoMasterDataType*>(&shm_ptr_->data[0]);
}

std::string hako::command::HakoMetaDumper::dump_json() {
    json root;

    root["shared_memory"] = {
        {"shm_id", shm_ptr_->shm_id},
        {"sem_id", shm_ptr_->sem_id},
        {"data_size", shm_ptr_->data_size}
    };

    const auto& m = *master_data_ptr_;
    root["master_data"] = {
        {"master_pid", m.master_pid},
        {"state", m.state},
        {"time_usec", {
            {"max_delay", m.time_usec.max_delay},
            {"delta", m.time_usec.delta},
            {"current", m.time_usec.current}
        }},
        {"asset_num", m.asset_num}
    };

    json assets_json = json::array();
    for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
        const auto& asset = m.assets[i];
        std::string name = std::string(asset.name.data);
        assets_json.push_back({
            {"id", asset.id},
            {"name", name},
            {"type", asset.type},
            {"callback", {
                {"start", reinterpret_cast<uintptr_t>(asset.callback.start)},
                {"stop", reinterpret_cast<uintptr_t>(asset.callback.stop)},
                {"reset", reinterpret_cast<uintptr_t>(asset.callback.reset)}
            }}
        });
    }
    root["master_data"]["assets"] = assets_json;

    // 必要に応じて assets_ev や pdu_meta_data も追記可！

    return root.dump(2);  // インデント2で整形出力
}

