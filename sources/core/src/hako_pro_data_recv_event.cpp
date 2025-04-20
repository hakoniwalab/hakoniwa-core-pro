#include "hako_pro_data.hpp"
#include <fstream>
#include <iostream>
using namespace hako::data;

bool hako::data::pro::HakoProData::register_data_recv_event(const std::string& robot_name, int channel_id, void (*on_recv)(int), int& recv_event_id)
{
    recv_event_id = -1;
    if (recv_event_table_ == nullptr) {
        std::cout << "ERROR: recv_event_table_ is null" << std::endl;
        return false;
    }
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robot_name, channel_id);
    if (real_id < 0) {
        std::cout << "ERROR: real_id is invalid: robot_name: " << robot_name << " channel_id: " << channel_id << std::endl;
        return false;
    }

    if (recv_event_table_->entry_num >= HAKO_RECV_EVENT_MAX) {
        std::cout << "ERROR: recv_event_table_ is full" << std::endl;
        return false;
    }
    bool ret = false;
    //this->shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_2);
    this->lock_memory();
    for (int i = 0; i < HAKO_RECV_EVENT_MAX; i++) {
        if (recv_event_table_->entries[i].enabled == false) {
            hako::core::context::HakoContext context;
            recv_event_table_->entries[i].enabled = true;
            recv_event_table_->entries[i].recv_flag = false;
            recv_event_table_->entries[i].proc_id = context.get_pid();
            recv_event_table_->entries[i].real_channel_id = real_id;
            if (on_recv != nullptr) {
                recv_event_table_->entries[i].type = HAKO_RECV_EVENT_TYPE_CALLBACK;
                recv_event_table_->entries[i].on_recv = on_recv;
            }
            else {
                recv_event_table_->entries[i].type = HAKO_RECV_EVENT_TYPE_FLAG;
                recv_event_table_->entries[i].on_recv = nullptr;
            }
            recv_event_id = i;
            recv_event_table_->entry_num++;
            ret = true;
            break;
        }
    }
    //this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
    this->unlock_memory();
    std::cout << "INFO: register_data_recv_event() robot_name: " << robot_name << " channel_id: " << channel_id << " ret: " << ret << std::endl;
    return ret;
}
bool hako::data::pro::HakoProData::get_recv_event(const char* asset_name, const std::string& robot_name, int channel_id, int& recv_event_id)
{
    int asset_id = -1;
    if (asset_name != nullptr)
    {
        auto* asset = this->master_data_->get_asset_nolock(asset_name);
        if (asset == nullptr) {
            return false;
        }
        asset_id = asset->id;
    }
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robot_name, channel_id);
    if (real_id < 0) {
        return false;
    }
    return get_recv_event(asset_id, real_id, recv_event_id);
}
bool hako::data::pro::HakoProData::get_recv_event(int asset_id, const std::string& robot_name, int channel_id, int& recv_event_id)
{
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robot_name, channel_id);
    if (real_id < 0) {
        return false;
    }
    return get_recv_event(asset_id, real_id, recv_event_id);
}
/*
 * if asset_id < 0 then it is external 
 * channel_id must be real channel id!
 */
bool hako::data::pro::HakoProData::get_recv_event(int asset_id, int channel_id, int& recv_event_id)
{
    recv_event_id = -1;
    if (recv_event_table_ == nullptr) {
        return false;
    }
    if (channel_id >= HAKO_PDU_CHANNEL_MAX) {
        return false;
    }
    if (recv_event_table_->entry_num == 0) {
        return false;
    }
    hako::core::context::HakoContext context;
    auto pid = context.get_pid();
    for (int i = 0; i < recv_event_table_->entry_num; i++) {
        if (recv_event_table_->entries[i].enabled == false) {
            continue;
        }
        if (recv_event_table_->entries[i].proc_id != pid) {
            continue;
        }
        if (recv_event_table_->entries[i].real_channel_id != channel_id) {
            continue;
        }
        this->master_data_->get_pdu_data()->read_pdu_spin_lock(asset_id, channel_id);
        bool recv_flag = recv_event_table_->entries[i].recv_flag;
        //std::cout << "INFO: get_recv_event() recv_flag: " << recv_flag << std::endl;
        if (recv_flag) {
            recv_event_table_->entries[i].recv_flag = false;
            recv_event_id = i;
        }
        this->master_data_->get_pdu_data()->read_pdu_spin_unlock(asset_id, channel_id);
        //std::cout << "INFO: get_recv_event() asset_id: " << asset_id << " channel_id: " << channel_id << " recv_event_id: " << recv_event_id << std::endl;
        return recv_flag;
    }
    return false;
}
bool hako::data::pro::HakoProData::call_recv_event_callbacks(const char* asset_name)
{
    //std::cout << "INFO: call_recv_event_callbacks() asset_name: " << asset_name << std::endl;
    if (recv_event_table_ == nullptr) {
        //std::cout << "ERROR: recv_event_table_ is null" << std::endl;
        return false;
    }
    if (recv_event_table_->entry_num == 0) {
        //std::cout << "ERROR: recv_event_table_ is empty" << std::endl;
        return false;
    }
    int asset_id = -1;
    if (asset_name != nullptr)
    {
        auto* asset = this->master_data_->get_asset_nolock(asset_name);
        if (asset == nullptr) {
            //std::cout << "ERROR: asset is null" << std::endl;
            return false;
        }
        asset_id = asset->id;
    }
    hako::core::context::HakoContext context;
    auto pid = context.get_pid();
    for (int i = 0; i < recv_event_table_->entry_num; i++) {
        if (recv_event_table_->entries[i].enabled == false) {
            continue;
        }
        if (recv_event_table_->entries[i].proc_id != pid) {
            continue;
        }
        if (recv_event_table_->entries[i].on_recv == nullptr) {
            continue;
        }
        this->master_data_->get_pdu_data()->read_pdu_spin_lock(asset_id, recv_event_table_->entries[i].real_channel_id);
        bool recv_flag = recv_event_table_->entries[i].recv_flag;
        if (recv_flag) {
            recv_event_table_->entries[i].recv_flag = false;
            //std::cout << "INFO: call_recv_event_callbacks() recv_flag: " << recv_flag << std::endl;
        }
        this->master_data_->get_pdu_data()->read_pdu_spin_unlock(asset_id, recv_event_table_->entries[i].real_channel_id);
        if (recv_flag && (recv_event_table_->entries[i].on_recv != nullptr)) {
            recv_event_table_->entries[i].on_recv(i);
        }
    }
    //std::cout << "INFO: call_recv_event_callbacks() end" << std::endl;
    return true;
}
bool hako::data::pro::HakoProData::call_recv_event_callback(int recv_event_id)
{
    if (recv_event_table_ == nullptr) {
        return false;
    }
    if (recv_event_id < 0 || recv_event_id >= HAKO_RECV_EVENT_MAX) {
        return false;
    }
    if (recv_event_table_->entries[recv_event_id].enabled == false) {
        return false;
    }
    if (recv_event_table_->entries[recv_event_id].type != HAKO_RECV_EVENT_TYPE_CALLBACK) {
        return false;
    }
    if (recv_event_table_->entries[recv_event_id].on_recv == nullptr) {
        return false;
    }
    recv_event_table_->entries[recv_event_id].on_recv(recv_event_id);
    return true;
}