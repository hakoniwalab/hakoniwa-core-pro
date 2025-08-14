#include "dump_meta.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>
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
    for (uint32_t i = 0; i < m.asset_num; i++) {
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
    json assets_ev_json = json::array();
    for (uint32_t i = 0; i < m.asset_num; i++) {
        const auto& asset_ev = m.assets_ev[i];
        assets_ev_json.push_back({
            {"pid", asset_ev.pid},
            {"ctime", asset_ev.ctime},
            {"update_time", asset_ev.update_time},
            {"event", asset_ev.event},
            {"event_feedback", asset_ev.event_feedback},
            {"semid", asset_ev.semid}
        });
    }
    root["master_data"]["assets_ev"] = assets_ev_json;


    const auto& meta = m.pdu_meta_data;
    root["master_data"]["pdu_meta_data"] = {
        {"mode", meta.mode},
        {"asset_num", meta.asset_num},
        {"pdu_sync_asset_id", meta.pdu_sync_asset_id},
        {"channel_num", meta.channel_num}
    };

    json check_status = json::array();
    for (uint32_t i = 0; i < m.asset_num; i++) {
        check_status.push_back(meta.asset_pdu_check_status[i]);
    }
    root["master_data"]["pdu_meta_data"]["asset_pdu_check_status"] = check_status;

    json channels = json::array();
    for (int i = 0; i < meta.channel_num; i++) {
        channels.push_back({
            {"real_cid", i},
            {"offset", meta.channel[i].offset},
            {"size", meta.channel[i].size}
        });
    }
    root["master_data"]["pdu_meta_data"]["real_channels"] = channels;

    json channel_maps = json::array();
    for (int i = 0; i < meta.channel_num; i++) {
        channel_maps.push_back({
            {"real_cid", i},
            {"robo_name", std::string(meta.channel_map[i].robo_name.data)},
            {"logical_channel_id", meta.channel_map[i].logical_channel_id}
        });
    }
    root["master_data"]["pdu_meta_data"]["channel_map"] = channel_maps;

    json flags_json;
    for (int i = 0; i < meta.channel_num; i++) {
        flags_json["is_dirty"].push_back(m.pdu_meta_data.is_dirty[i]);
        flags_json["is_wbusy"].push_back(m.pdu_meta_data.is_wbusy[i]);
        flags_json["is_rbusy_for_external"].push_back(m.pdu_meta_data.is_rbusy_for_external[i]);
    }

    json rbusy_json;
    for (uint32_t aid = 0; aid < m.asset_num; aid++) {
        json asset_rbusy;
        for (int cid = 0; cid < meta.channel_num; cid++) {
            asset_rbusy.push_back(m.pdu_meta_data.is_rbusy[aid][cid]);
        }
        rbusy_json[std::string(m.assets[aid].name.data)] = asset_rbusy;
    }
    flags_json["is_rbusy"] = rbusy_json;

    root["master_data"]["pdu_meta_data"]["flags"] = flags_json;

    json version_json;
    json read_version_json;
    for (uint32_t aid = 0; aid < m.asset_num; aid++) {
        json versions;
        for (int cid = 0; cid < meta.channel_num; cid++) {
            versions.push_back(m.pdu_meta_data.pdu_read_version[aid][cid]);
        }
        read_version_json[std::string(m.assets[aid].name.data)] = versions;
    }
    version_json["pdu_read_version"] = read_version_json;
    
    json write_version_json = json::array();
    for (int cid = 0; cid < meta.channel_num; cid++) {
        write_version_json.push_back(m.pdu_meta_data.pdu_write_version[cid]);
    }
    version_json["pdu_write_version"] = write_version_json;
    
    root["master_data"]["pdu_meta_data"]["version"] = version_json;
    return root.dump(2);
}

