#include "hako_pro_data.hpp"

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

bool pro::HakoProAssetExtension::on_pdu_data_before_write(int real_channel_id) 
{
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_before_write()" << std::endl;
    HakoRecvEventTableType *table = this->pro_->get_recv_event_table();
    if (table == nullptr) {
        std::cout << "ERROR: HakoProAssetExtension::on_pdu_data_before_write() table is null" << std::endl;
        return false;
    }
    //process lock is not needed because upper layer is already locked
    for (int i = 0; i < table->entry_num; ++i) {
        if (table->entries[i].enabled && (table->entries[i].real_channel_id == real_channel_id)
            && (table->entries[i].recv_flag)) {
                //busy
                return false;
        }
    }
    //std::cout << "INFO: HakoProAssetExtension::on_pdu_data_before_write() end" << std::endl;
    return true;
}

/*
 * Service server API
 */
int hako::data::pro::HakoProData::create_service(const std::string& serviceName)
{
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null" << std::endl;
        return -1;
    }
    if (service_table_->entry_num >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_table_ is full" << std::endl;
        return -1;
    }
    if (this->is_exist_service(serviceName)) {
        std::cerr << "ERROR: service already exists" << std::endl;
        return -1;
    }
    int service_id = -1;
    for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
        if (service_table_->entries[i].enabled == true) {
            continue;
        }
        for (int j = 0; j < service_table_->entries[i].maxClients; j++) {
            int recv_event_id = -1;
            int server_channel_id = HAKO_SERVICE_SERVER_CHANNEL_ID + (HAKO_SERVICE_SERVER_CHANNEL_ID_MAX * j);
            bool ret = this->register_data_recv_event(serviceName, server_channel_id, nullptr, recv_event_id);
            if (ret == false) {
                std::cerr << "ERROR: Failed to register data receive event for service server" << std::endl;
                return -1;
            }
            service_table_->entries[i].clientChannelMap[j].requestChannelId = server_channel_id;
            service_table_->entries[i].clientChannelMap[j].requestRecvEventId = recv_event_id;
            std::cout << "INFO: register_data_recv_event() serviceName: "
                << serviceName << " channel_id: " << server_channel_id
                << " recv_event_id: " << recv_event_id << std::endl;
        }
        service_table_->entries[i].enabled = true;
        memcpy(service_table_->entries[i].serviceName, serviceName.c_str(), serviceName.length());
        service_table_->entries[i].serviceName[serviceName.length()] = '\0';
        service_table_->entry_num++;
        service_id = i;
        std::cout << "INFO: service_id: " << service_id << " serviceName: " << service_table_->entries[i].serviceName << std::endl;
        break;
    }
    return service_id;
}

int hako::data::pro::HakoProData::get_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null" << std::endl;
        return -1;
    }
    if (service_id < 0 || service_id >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_id is invalid" << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len <= 0) {
        std::cerr << "ERROR: packet is null or packet_len is invalid" << std::endl;
        return -1;
    }
    if (service_table_->entries[service_id].enabled == false) {
        std::cerr << "ERROR: service is not enabled" << std::endl;
        return -1;
    }
    if (client_id < 0 || client_id >= service_table_->entries[service_id].maxClients) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return -1;
    }
    int channel_id = service_table_->entries[service_id].clientChannelMap[client_id].requestChannelId;
    int recv_event_id = service_table_->entries[service_id].clientChannelMap[client_id].requestRecvEventId;
    char* serviceName = service_table_->entries[service_id].serviceName;
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(serviceName, channel_id);
    if (real_id < 0) {
        std::cerr << "ERROR: real_id is invalid" << std::endl;
        return -1;
    }
    auto ret = this->get_recv_event(asset_id, real_id, recv_event_id);
    if (ret == false) {
        //not found
        return -1;
    }
    bool read_result = false;
    if (asset_id >= 0) {
        read_result = master_data_->get_pdu_data()->read_pdu(asset_id, real_id, packet, packet_len);
    }
    else {
        read_result = master_data_->get_pdu_data()->read_pdu_for_external(real_id, packet, packet_len);
    }
    if (read_result == false) {
        std::cerr << "ERROR: get_request() failed to read pdu" << std::endl;
        return -1;
    }
    return 0;
}

int hako::data::pro::HakoProData::put_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    (void)asset_id; // asset_id is not used in this function
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null" << std::endl;
        return -1;
    }
    if (service_id < 0 || service_id >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_id is invalid" << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len <= 0) {
        std::cerr << "ERROR: packet is null or packet_len is invalid" << std::endl;
        return -1;
    }
    if (service_table_->entries[service_id].enabled == false) {
        std::cerr << "ERROR: service is not enabled" << std::endl;
        return -1;
    }
    if (client_id < 0 || client_id >= service_table_->entries[service_id].maxClients) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return -1;
    }
    int channel_id = service_table_->entries[service_id].clientChannelMap[client_id].responseChannelId;
    char* serviceName = service_table_->entries[service_id].serviceName;
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(serviceName, channel_id);
    if (real_id < 0) {
        std::cerr << "ERROR: real_id is invalid" << std::endl;
        return -1;
    }
    bool write_result = false;
    write_result = master_data_->get_pdu_data()->write_pdu(real_id, packet, packet_len);
    if (write_result == false) {
        std::cerr << "ERROR: put_response() failed to write pdu" << std::endl;
        return -1;
    }
    return 0;
}
