#include "hako_pro_data.hpp"
#include "hako_pro.hpp"
#include "utils/hako_config_loader.hpp"
#include "utils/hako_share/impl/hako_flock.hpp"
#include <fstream>
#include <iostream>
using namespace hako::data;
using hako::data::pro::HakoRecvEventTableType;
using hako::data::pro::HakoServiceTableType;

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

/*
 * Creates the tables empty. Caller holds the pro init lock.
 */
static bool create_empty_tables(pro::HakoProData& pro)
{
    auto shmid = pro.get_shared_memory()->create_memory(HAKO_SHARED_MEMORY_ID_2, TOTAL_HAKO_PRO_DATA_SIZE);
    if (shmid < 0) {
        return false;
    }
    char* datap = (char*)pro.lock_memory();
    pro.set_recv_event_table(reinterpret_cast<HakoRecvEventTableType*>(&datap[OFFSET_HAKO_RECV_EVENT_TABLE]));
    pro.set_service_table(reinterpret_cast<HakoServiceTableType*>(&datap[OFFSET_HAKO_SERVICE_TABLE]));
    memset(&datap[OFFSET_HAKO_RECV_EVENT_TABLE], 0, sizeof(HakoRecvEventTableType));
    memset(&datap[OFFSET_HAKO_SERVICE_TABLE], 0, sizeof(HakoServiceTableType));
    // create_memory keeps an existing file's contents, so clear the run info as well.
    memset(&datap[OFFSET_HAKO_SERVICE_TABLE + sizeof(HakoServiceTableType)], 0, sizeof(pro::HakoProRunInfoType));
    pro.unlock_memory();
    return true;
}

/*
 * Clears what belongs to one run (received/pending flags) and keeps the
 * configuration (services, client slots, recv event registrations). Each flag is
 * cleared under its channel's PDU read spin lock, the lock get_recv_event uses and
 * write_pdu waits for, so a request or response being written is not lost halfway
 * (#109). The tables' atomic_is_busy fields are spin locks and are left alone.
 */
void pro::HakoProData::clear_run_flags()
{
    if (this->recv_event_table_ == nullptr) {
        return;
    }
    auto pdu_data = this->master_data_->get_pdu_data();
    this->lock_recv_event_table();
    for (int i = 0; i < this->recv_event_table_->entry_num; ++i) {
        auto& entry = this->recv_event_table_->entries[i];
        if (!entry.enabled) {
            continue;
        }
        pdu_data->read_pdu_spin_lock(-1, entry.real_channel_id);
        entry.recv_flag = false;
        entry.pending_flag = false;
        pdu_data->read_pdu_spin_unlock(-1, entry.real_channel_id);
    }
    this->unlock_recv_event_table();
}

bool pro::HakoProData::create_session_memory()
{
    HakoFlockObjectType* init_lock = acquire_pro_init_lock();
    if (init_lock == nullptr) {
        return false;
    }
    bool ret = create_empty_tables(*this);
    if (ret) {
        this->rebuild_service_name_indexes();
    }
    release_pro_init_lock(init_lock);
    if (!ret) {
        std::cout << "ERROR: HakoProData::create_session_memory() failed to create memory" << std::endl;
        return false;
    }
    std::cout << "INFO: HakoProData::create_session_memory() created memory" << std::endl;
    return true;
}

bool pro::HakoProData::on_pdu_data_create()
{
    HakoFlockObjectType* init_lock = acquire_pro_init_lock();
    if (init_lock == nullptr) {
        return false;
    }
    std::cout << "INFO: HakoProData::on_pdu_data_create()" << std::endl;
    if (this->service_table_ == nullptr) {
        char* datap = (char*)this->get_shared_memory()->load_memory(HAKO_SHARED_MEMORY_ID_2, TOTAL_HAKO_PRO_DATA_SIZE);
        if (datap != nullptr) {
            this->set_recv_event_table(reinterpret_cast<HakoRecvEventTableType*>(&datap[OFFSET_HAKO_RECV_EVENT_TABLE]));
            this->set_service_table(reinterpret_cast<HakoServiceTableType*>(&datap[OFFSET_HAKO_SERVICE_TABLE]));
        }
        else if (!create_empty_tables(*this)) {
            // No conductor-created tables (older conductor): create them as before.
            std::cout << "ERROR: HakoProData::on_pdu_data_create() failed to create memory" << std::endl;
            release_pro_init_lock(init_lock);
            return false;
        }
    }
    // New generation first: a response of the previous run written after this is
    // refused by its server (generation check), one written before has its flag
    // cleared below.
    this->next_run_generation();
    this->clear_run_flags();
    this->rebuild_service_name_indexes();
    release_pro_init_lock(init_lock);
    std::cout << "INFO: HakoProData::on_pdu_data_create() kept service registrations" << std::endl;
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
    // Keep the registrations (#109); only one run's state is cleared.
    this->clear_run_flags();
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
