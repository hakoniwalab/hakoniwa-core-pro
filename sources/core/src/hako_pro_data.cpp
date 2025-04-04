#include "hako_pro_data.hpp"

using namespace hako::data;

bool pro::HakoProMasterExtension::on_pdu_data_create()
{
    auto shmid = this->pro_->get_shared_memory()->create_memory(HAKO_SHARED_MEMORY_ID_2, sizeof(HakoRecvEventTableType));
    if (shmid < 0) {
        return false;
    }
    (void)shmid;
    void *datap = this->pro_->get_shared_memory()->lock_memory(HAKO_SHARED_MEMORY_ID_2);
    this->pro_->set_recv_event_table(static_cast<HakoRecvEventTableType*>(datap));
    {
        memset(datap, 0, sizeof(HakoRecvEventTableType));
    }
    this->pro_->get_shared_memory()->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
    return true;
}
bool pro::HakoProMasterExtension::on_pdu_data_load()
{
    void *datap = this->pro_->get_shared_memory()->load_memory(HAKO_SHARED_MEMORY_ID_2, sizeof(HakoRecvEventTableType));
    if (datap == nullptr) {
        return false;
    }
    this->pro_->set_recv_event_table(static_cast<HakoRecvEventTableType*>(datap));
    return true;
}