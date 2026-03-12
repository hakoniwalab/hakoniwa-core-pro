#include "hako_pro_data.hpp"
#include "hako_pro.hpp"
#include "utils/hako_config_loader.hpp"
#include "utils/hako_share/impl/hako_flock.hpp"
#include <fstream>
#include <iostream>
using namespace hako::data;

namespace {
static std::string get_pro_init_lock_filepath()
{
    char buf[4096];
    HakoConfigType config;
    hako_config_load(config);

    if (config.param == nullptr) {
        snprintf(buf, sizeof(buf), "./pro_init.lock");
    }
    else {
        std::string core_mmap_path = config.param["core_mmap_path"];
        snprintf(buf, sizeof(buf), "%s/pro_init.lock", core_mmap_path.c_str());
    }
    return std::string(buf);
}

static HakoFlockObjectType* acquire_pro_init_lock()
{
    std::string filepath = get_pro_init_lock_filepath();
    HakoFlockObjectType* lock = hako_flock_create(filepath);
    if (lock == nullptr) {
        std::cout << "ERROR: failed to create pro init lock file: " << filepath << std::endl;
        return nullptr;
    }
    hako_flock_acquire(lock);
    return lock;
}

static void release_pro_init_lock(HakoFlockObjectType* lock)
{
    if (lock == nullptr) {
        return;
    }
    hako_flock_release(lock);
    hako_flock_destroy(lock);
}
}

bool pro::HakoProData::on_pdu_data_create()
{
    HakoFlockObjectType* init_lock = acquire_pro_init_lock();
    if (init_lock == nullptr) {
        return false;
    }
    std::cout << "INFO: HakoProData::on_pdu_data_create()" << std::endl;
    auto shmid = this->get_shared_memory()->create_memory(HAKO_SHARED_MEMORY_ID_2, TOTAL_HAKO_PRO_DATA_SIZE);
    if (shmid < 0) {
        std::cout << "ERROR: HakoProData::on_pdu_data_create() failed to create memory" << std::endl;
        release_pro_init_lock(init_lock);
        return false;
    }
    (void)shmid;
    char* datap = (char*)this->lock_memory();
    this->set_recv_event_table(reinterpret_cast<HakoRecvEventTableType*>(&datap[OFFSET_HAKO_RECV_EVENT_TABLE]));
    this->set_service_table(reinterpret_cast<HakoServiceTableType*>(&datap[OFFSET_HAKO_SERVICE_TABLE]));
    {
        memset(&datap[OFFSET_HAKO_RECV_EVENT_TABLE], 0, sizeof(HakoRecvEventTableType));
        memset(&datap[OFFSET_HAKO_SERVICE_TABLE], 0, sizeof(HakoServiceTableType));
        //this->set_service_data();
        this->service_table_->entry_num = 0;
        this->rebuild_service_name_indexes();
    }
    this->unlock_memory();
    release_pro_init_lock(init_lock);
    std::cout << "INFO: HakoProData::on_pdu_data_create() created memory" << std::endl;
    return true;
}
bool pro::HakoProData::on_pdu_data_load()
{
    HakoFlockObjectType* init_lock = acquire_pro_init_lock();
    if (init_lock == nullptr) {
        return false;
    }
    std::cout << "INFO: HakoProData::on_pdu_data_load()" << std::endl;
    char *datap = (char*)this->get_shared_memory()->load_memory(HAKO_SHARED_MEMORY_ID_2, TOTAL_HAKO_PRO_DATA_SIZE);
    if (datap == nullptr) {
        std::cout << "ERROR: HakoProData::on_pdu_data_load() failed to load memory" << std::endl;
        release_pro_init_lock(init_lock);
        return false;
    }
    this->set_recv_event_table(reinterpret_cast<HakoRecvEventTableType*>(&datap[OFFSET_HAKO_RECV_EVENT_TABLE]));
    this->set_service_table(reinterpret_cast<HakoServiceTableType*>(&datap[OFFSET_HAKO_SERVICE_TABLE]));
    this->rebuild_service_name_indexes();
    release_pro_init_lock(init_lock);
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
    this->rebuild_service_name_indexes();
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
    bool ret_value = false;
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
            ret_value = true;
        }
    }
    //std::cout << "ERROR: HakoProAssetExtension::on_pdu_data_write(): can not set rcv flag..." << std::endl;
    return ret_value;
}

bool pro::HakoProAssetExtension::on_pdu_data_before_write(int real_channel_id) 
{
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_before_write()" << std::endl;
    HakoRecvEventTableType *table = this->pro_->get_recv_event_table();
    if (table == nullptr) {
        std::cout << "ERROR: HakoProAssetExtension::on_pdu_data_before_write() table is null" << std::endl;
        return false;
    }
    //auto now = get_timestamp();
    //process lock is not needed because upper layer is already locked
    for (int i = 0; i < table->entry_num; ++i) {
        if (table->entries[i].enabled && (table->entries[i].real_channel_id == real_channel_id)
            && (table->entries[i].recv_flag) && (table->entries[i].pending_flag == false)) {
                //busy
                //std::cout << now << ": HakoProAssetExtension::on_pdu_data_before_write() busy... real_channel_id: " << real_channel_id << std::endl;
                return false;
        }
    }
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_before_write() end" << std::endl;
    return true;
}
