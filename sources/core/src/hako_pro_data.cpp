#include "hako_pro_data.hpp"

using namespace hako::data;

bool pro::HakoProData::on_pdu_data_create()
{
    std::cout << "INFO: HakoProData::on_pdu_data_create()" << std::endl;
    auto shmid = this->get_shared_memory()->create_memory(HAKO_SHARED_MEMORY_ID_2, sizeof(HakoRecvEventTableType));
    if (shmid < 0) {
        std::cout << "ERROR: HakoProData::on_pdu_data_create() failed to create memory" << std::endl;
        return false;
    }
    (void)shmid;
    void *datap = this->get_shared_memory()->lock_memory(HAKO_SHARED_MEMORY_ID_2);
    this->set_recv_event_table(static_cast<HakoRecvEventTableType*>(datap));
    {
        memset(datap, 0, sizeof(HakoRecvEventTableType));
    }
    this->get_shared_memory()->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
    std::cout << "INFO: HakoProData::on_pdu_data_create() created memory" << std::endl;
    return true;
}
bool pro::HakoProData::on_pdu_data_load()
{
    std::cout << "INFO: HakoProData::on_pdu_data_load()" << std::endl;
    void *datap = this->get_shared_memory()->load_memory(HAKO_SHARED_MEMORY_ID_2, sizeof(HakoRecvEventTableType));
    if (datap == nullptr) {
        std::cout << "ERROR: HakoProData::on_pdu_data_load() failed to load memory" << std::endl;
        return false;
    }
    this->set_recv_event_table(static_cast<HakoRecvEventTableType*>(datap));
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
    //process lock is not needed because upper layer is already locked
    for (int i = 0; i < table->entry_num; ++i) {
#if false
        std::cout << "INFO: HakoProAssetExtension::on_pdu_data_write() table->entries[" << i << "]" 
                    << " enabled: " << table->entries[i].enabled
                    << " proc_id: " << table->entries[i].proc_id
                    << " real_channel_id: " << table->entries[i].real_channel_id
                    << " recv_flag: " << table->entries[i].recv_flag
                    << " type: " << table->entries[i].type
        << std::endl;
#endif
        if (table->entries[i].enabled && (table->entries[i].real_channel_id == real_channel_id)) {
            table->entries[i].recv_flag = true;
            //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_write() real_channel_id: " << real_channel_id << std::endl;
            break;
        }
    }
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_write() end" << std::endl;
    return true;
}
