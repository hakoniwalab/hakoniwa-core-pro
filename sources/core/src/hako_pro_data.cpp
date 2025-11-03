#include "hako_pro_data.hpp"
#include "hako_pro.hpp"
#include <fstream>
#include <iostream>
using namespace hako::data;

bool pro::HakoProData::on_pdu_data_create()
{
    std::cout << "INFO: HakoProData::on_pdu_data_create()" << std::endl;
    auto shmid = this->get_shared_memory()->create_memory(HAKO_SHARED_MEMORY_ID_2, TOTAL_HAKO_PRO_DATA_SIZE);
    if (shmid < 0) {
        std::cout << "ERROR: HakoProData::on_pdu_data_create() failed to create memory" << std::endl;
        return false;
    }
    (void)shmid;
    //char *datap = (char*)this->get_shared_memory()->lock_memory(HAKO_SHARED_MEMORY_ID_2);
    char* datap = (char*)this->lock_memory();
    this->set_recv_event_table(reinterpret_cast<HakoRecvEventTableType*>(&datap[OFFSET_HAKO_RECV_EVENT_TABLE]));
    this->set_service_table(reinterpret_cast<HakoServiceTableType*>(&datap[OFFSET_HAKO_SERVICE_TABLE]));
    {
        memset(&datap[OFFSET_HAKO_RECV_EVENT_TABLE], 0, sizeof(HakoRecvEventTableType));
        memset(&datap[OFFSET_HAKO_SERVICE_TABLE], 0, sizeof(HakoServiceTableType));
        //this->set_service_data();
        this->service_table_->entry_num = 0;
    }
    //this->get_shared_memory()->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
    this->unlock_memory();
    std::cout << "INFO: HakoProData::on_pdu_data_create() created memory" << std::endl;
    return true;
}
bool pro::HakoProData::on_pdu_data_load()
{
    std::cout << "INFO: HakoProData::on_pdu_data_load()" << std::endl;
    char *datap = (char*)this->get_shared_memory()->load_memory(HAKO_SHARED_MEMORY_ID_2, TOTAL_HAKO_PRO_DATA_SIZE);
    if (datap == nullptr) {
        std::cout << "ERROR: HakoProData::on_pdu_data_load() failed to load memory" << std::endl;
        return false;
    }
    this->set_recv_event_table(reinterpret_cast<HakoRecvEventTableType*>(&datap[OFFSET_HAKO_RECV_EVENT_TABLE]));
    this->set_service_table(reinterpret_cast<HakoServiceTableType*>(&datap[OFFSET_HAKO_SERVICE_TABLE]));
    std::cout << "INFO: HakoProData::on_pdu_data_load() loaded memory" << std::endl;
    return true;
}
bool pro::HakoProData::on_pdu_data_reset()
{
    HakoRecvEventTableType *table = this->get_recv_event_table();
    if (table == nullptr) {
        return false;
    }
    table->entry_num = 0;
    for (int i = 0; i < HAKO_RECV_EVENT_MAX; ++i) {
        table->entries[i].enabled = false;
    }
    return true;
}
bool pro::HakoProData::on_pdu_data_destroy()
{
    this->destroy();
    return true;
}
bool pro::HakoProAssetExtension::on_pdu_data_write(int real_channel_id) 
{
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_write()" << std::endl;
    HakoRecvEventTableType *table = this->pro_->get_recv_event_table();
    if (table == nullptr) {
        std::cout << "ERROR: HakoProAssetExtension::on_pdu_data_write() table is null" << std::endl;
        return false;
    }
    auto now = hako::data::pro::get_timestamp();
    //process lock is not needed because upper layer is already locked
    //std::cout << now << ": HakoProAssetExtension::on_pdu_data_write() table->entry_num: " << table->entry_num << std::endl;
    for (int i = 0; i < table->entry_num; ++i) {
#if false
        std::cout << "INFO: HakoProAssetExtension::on_pdu_data_write() table->entries[" << i << "]" 
                    << " enabled: " << table->entries[i].enabled
                    << " proc_id: " << table->entries[i].proc_id
                    << " table: real_channel_id: " << table->entries[i].real_channel_id
                    << " args: real_channel_id: " << real_channel_id
                    << " recv_flag: " << table->entries[i].recv_flag
                    << " type: " << table->entries[i].type
        << std::endl;
#endif
        if (table->entries[i].enabled && (table->entries[i].real_channel_id == real_channel_id)) {
            table->entries[i].recv_flag = true;
            //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_write() real_channel_id: " << real_channel_id << std::endl;
            return true;
        }
    }
    //std::cout << "ERROR: HakoProAssetExtension::on_pdu_data_write(): can not set rcv flag..." << std::endl;
    return false;
}

bool pro::HakoProAssetExtension::on_pdu_data_before_write(int real_channel_id) 
{
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_before_write()" << std::endl;
    HakoRecvEventTableType *table = this->pro_->get_recv_event_table();
    if (table == nullptr) {
        std::cout << "ERROR: HakoProAssetExtension::on_pdu_data_before_write() table is null" << std::endl;
        return false;
    }
    auto now = get_timestamp();
    //process lock is not needed because upper layer is already locked
    for (int i = 0; i < table->entry_num; ++i) {
        if (table->entries[i].enabled && (table->entries[i].real_channel_id == real_channel_id)
            && (table->entries[i].recv_flag)) {
                //busy
                std::cout << now << ": HakoProAssetExtension::on_pdu_data_before_write() busy... real_channel_id: " << real_channel_id << std::endl;
                return false;
        }
    }
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_before_write() end" << std::endl;
    return true;
}
