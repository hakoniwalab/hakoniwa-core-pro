#include "hako_pro_data.hpp"

using namespace hako::data;

bool pro::HakoProData::on_pdu_data_create()
{
    std::cout << "INFO: HakoProData::on_pdu_data_create()" << std::endl;
    auto shmid = this->get_shared_memory()->create_memory(HAKO_SHARED_MEMORY_ID_2, sizeof(HakoRecvEventTableType));
    if (shmid < 0) {
        return false;
    }
    (void)shmid;
    void *datap = this->get_shared_memory()->lock_memory(HAKO_SHARED_MEMORY_ID_2);
    this->set_recv_event_table(static_cast<HakoRecvEventTableType*>(datap));
    {
        memset(datap, 0, sizeof(HakoRecvEventTableType));
    }
    this->get_shared_memory()->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
    return true;
}
bool pro::HakoProData::on_pdu_data_load()
{
    void *datap = this->get_shared_memory()->load_memory(HAKO_SHARED_MEMORY_ID_2, sizeof(HakoRecvEventTableType));
    if (datap == nullptr) {
        return false;
    }
    this->set_recv_event_table(static_cast<HakoRecvEventTableType*>(datap));
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
    HakoRecvEventTableType *table = this->pro_->get_recv_event_table();
    if (table == nullptr) {
        return false;
    }
    //process lock is not needed because upper layer is already locked
    for (int i = 0; i < table->entry_num; ++i) {
        if (table->entries[i].enabled && (table->entries[i].real_channel_id == real_channel_id)) {
            table->entries[i].recv_flag = true;
            break;
        }
    }
    return true;
}
