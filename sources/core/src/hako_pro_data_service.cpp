#include "hako_pro_data.hpp"
#include <fstream>
#include <iostream>
using namespace hako::data;


bool hako::data::pro::HakoProData::is_exist_service(const std::string& service_name)
{
    for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
        if (service_table_->entries[i].enabled == false) {
            continue;
        }
        if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) == 0) {
            return true;
        }
    }
    return false;
}
bool hako::data::pro::HakoProData::is_exist_client_on_service(const std::string& service_name, const std::string& client_name)
{
    for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
        if (service_table_->entries[i].enabled == false) {
            continue;
        }
        if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) != 0) {
            continue;
        }
        for (int j = 0; j < service_table_->entries[i].maxClients; j++) {
            if (service_table_->entries[i].clientChannelMap[j].enabled == false) {
                continue;
            }
            if (strcmp(service_table_->entries[i].clientChannelMap[j].clientName, client_name.c_str()) == 0) {
                return true;
            }
        }
    }
    return false;
}
hako::data::pro::HakoServiceEntryTye& hako::data::pro::HakoProData::get_service_entry(const std::string& service_name)
{
    for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
        if (service_table_->entries[i].enabled == false) {
            continue;
        }
        if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) == 0) {
            return service_table_->entries[i];
        }
    }
    throw std::runtime_error("Service not found");
}
int hako::data::pro::HakoProData::get_service_id(const std::string& service_name)
{
    for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
        if (service_table_->entries[i].enabled == false) {
            continue;
        }
        if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Service API
 */

 bool hako::data::pro::HakoProData::initialize_service(const std::string& service_config_path)
 {
     if (service_config_path.empty()) {
         std::cerr << "ERROR: service_config_path is empty" << std::endl;
         return false;
     }
     std::ifstream ifs(service_config_path);
     
     if (!ifs.is_open()) {
         std::cerr << "Error: Failed to open service config file: " << service_config_path << std::endl;
         return false;
     }
 
     try {
         ifs >> this->service_impl_.param;
         int pduMetaDataSize = this->service_impl_.param["pduMetaDataSize"];
         for (const auto& item : this->service_impl_.param["services"]) {
             hako::data::pro::Service s;
             s.name = item["name"];
             s.type = item["type"];
             s.maxClients = item["maxClients"];
 
             const auto& pduSize = item["pduSize"];
             s.pdu_size_server_base = pduSize["server"]["baseSize"];
             s.pdu_size_client_base = pduSize["client"]["baseSize"];
             s.pdu_size_server_heap = pduSize["server"]["heapSize"];
             s.pdu_size_client_heap = pduSize["client"]["heapSize"];
 
             //create pdu channels for service
             s.server_total_size = pduMetaDataSize + s.pdu_size_server_base + s.pdu_size_server_heap;
             s.client_total_size = pduMetaDataSize + s.pdu_size_client_base + s.pdu_size_client_heap;
             for (int i = 0; i < s.maxClients; i++) {
                 int server_channel_id = HAKO_SERVICE_SERVER_CHANNEL_ID + (HAKO_SERVICE_SERVER_CHANNEL_ID_MAX * i);
                 bool ret = this->master_data_->get_pdu_data()->create_lchannel(s.name, server_channel_id, s.server_total_size);
                 if (ret == false) {
                     std::cerr << "Error: Failed to create PDU channel for service server: " << s.name << std::endl;
                     return false;
                 }
                 std::cout << "INFO: create_lchannel() serviceName: "
                     << s.name << " channel_id: " << server_channel_id
                     << " total_size: " << s.server_total_size
                     << std::endl;
                 int client_channel_id = HAKO_SERVICE_CLIENT_CHANNEL_ID + (HAKO_SERVICE_SERVER_CHANNEL_ID_MAX * i);
                 ret = this->master_data_->get_pdu_data()->create_lchannel(s.name, client_channel_id, s.client_total_size);
                 if (ret == false) {
                     std::cerr << "Error: Failed to create PDU channel for service client: " << s.name << std::endl;
                     return false;
                 }
                 std::cout << "INFO: create_lchannel() serviceName: "
                     << s.name << " channel_id: " << client_channel_id
                     << " total_size: " << s.client_total_size
                     << std::endl;
             }
             this->service_impl_.services.push_back(s);
         }
     } catch (const std::exception& e) {
         std::cerr << "Error: Failed to parse service config JSON: " << e.what() << std::endl;
         return -1;
     }
     ifs.close();
     
     return true;
 }
 void hako::data::pro::HakoProData::set_service_data()
 {
     //std::cout << "INFO: set_service_data()" << std::endl;
     if (service_table_ == nullptr) {
         std::cerr << "ERROR: service_table_ is null on set_service_data()" << std::endl;
         return;
     }
     service_table_->entry_num = this->service_impl_.services.size();
     //std::cout << "INFO: service_table_->entry_num: " << service_table_->entry_num << std::endl;
     for (int i = 0; i < service_table_->entry_num; i++) {
        //std::cout << "INFO: service_table_->entries[" << i << "]" << std::endl;
        auto& service = this->service_impl_.services[i];
        service_table_->entries[i].enabled = true;
        service_table_->entries[i].maxClients = service.maxClients;
        service_table_->entries[i].pdu_size_request = service.server_total_size;
        service_table_->entries[i].pdu_size_response = service.client_total_size;
        //std::cout << "INFO: service_table_->entries[" << i << "].pdu_size_request: " << service_table_->entries[i].pdu_size_request << std::endl;
        //std::cout << "INFO: service_table_->entries[" << i << "].pdu_size_response: " << service_table_->entries[i].pdu_size_response << std::endl;
        if (service.name.length() >= HAKO_SERVICE_NAMELEN_MAX) {
            std::cerr << "ERROR: service name is too long" << std::endl;
            break;
        }
        //std::cout << "INFO: service_table_->entries[" << i << "].serviceName: " << service.name << std::endl;
        service_table_->entries[i].namelen = service.name.length();
        memcpy(service_table_->entries[i].serviceName, service.name.c_str(), service.name.length());
        service_table_->entries[i].serviceName[service.name.length()] = '\0';
        for (int j = 0; j < service.maxClients; j++) {
            service_table_->entries[i].clientChannelMap[j].enabled = false;
            service_table_->entries[i].clientChannelMap[j].requestChannelId = HAKO_SERVICE_SERVER_CHANNEL_ID + (HAKO_SERVICE_SERVER_CHANNEL_ID_MAX * j);
            service_table_->entries[i].clientChannelMap[j].responseChannelId = HAKO_SERVICE_CLIENT_CHANNEL_ID + (HAKO_SERVICE_SERVER_CHANNEL_ID_MAX * j);
        }
    }
    //debug
#if false
    for (int i = 0; i < service_table_->entry_num; i++) {
        std::cout << "INFO: service_table_[" << i << "].serviceName: " << service_table_->entries[i].serviceName << std::endl;
        std::cout << "INFO: service_table_[" << i << "].maxClients: " << service_table_->entries[i].maxClients << std::endl;
        std::cout << "INFO: service_table_[" << i << "].pdu_size_request: " << service_table_->entries[i].pdu_size_request << std::endl;
        std::cout << "INFO: service_table_[" << i << "].pdu_size_response: " << service_table_->entries[i].pdu_size_response << std::endl;
        for (int j = 0; j < service_table_->entries[i].maxClients; j++) {
            std::cout << "INFO: service_table_[" << i << "].clientChannelMap[" << j << "].requestChannelId: "
                << service_table_->entries[i].clientChannelMap[j].requestChannelId << std::endl;
            std::cout << "INFO: service_table_[" << i << "].clientChannelMap[" << j << "].responseChannelId: "
                << service_table_->entries[i].clientChannelMap[j].responseChannelId << std::endl;
        }
    }
    std::cout << "INFO: set_service_data() done" << std::endl;
#endif
}
/*
 * Service server API
 */
int hako::data::pro::HakoProData::create_service(const std::string& serviceName)
{
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null on create_service()" << std::endl;
        return -1;
    }
    if (service_table_->entry_num == 0) {
        this->set_service_data();        
    }
    if (service_table_->entry_num >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_table_ is full on create_service()" << std::endl;
        return -1;
    }
    int service_id = get_service_id(serviceName);
    if (service_id < 0) {
        std::cerr << "ERROR: service_id is invalid on create_service()" << std::endl;
        return -1;
    }
    //std::cout << "INFO: create_service() service_id: " << service_id << std::endl;
    HakoServiceEntryTye& service_entry = service_table_->entries[service_id];
    for (int j = 0; j < service_entry.maxClients; j++) {
        //std::cout << "INFO: create_service() client_id: " << j << std::endl;
        int recv_event_id = -1;
        int server_channel_id = HAKO_SERVICE_SERVER_CHANNEL_ID + (HAKO_SERVICE_SERVER_CHANNEL_ID_MAX * j);
        bool ret = this->register_data_recv_event(serviceName, server_channel_id, nullptr, recv_event_id);
        if (ret == false) {
            std::cerr << "ERROR: Failed to register data receive event for service server" << std::endl;
            return -1;
        }
        service_entry.clientChannelMap[j].requestChannelId = server_channel_id;
        service_entry.clientChannelMap[j].requestRecvEventId = recv_event_id;
#if false
        std::cout << "INFO: register_data_recv_event() serviceName: "
            << serviceName << " channel_id: " << server_channel_id
            << " recv_event_id: " << recv_event_id << std::endl;
#endif
    }
    return service_id;
}

int hako::data::pro::HakoProData::get_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null on get_request()" << std::endl;
        return -1;
    }
    if (service_id < 0 || service_id >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_id is invalid on get_request()" << std::endl;
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
        std::cerr << "ERROR: service_table_ is null on put_request()" << std::endl;
        return -1;
    }
    if (service_id < 0 || service_id >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_id is invalid on put_request()" << std::endl;
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

/*
 * Service client API
 */
int hako::data::pro::HakoProData::create_service_client(const std::string& serviceName, const std::string& clientName, int& client_id)
{
    client_id = -1;
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null on create_service_client()" << std::endl;
        return -1;
    }
    if (service_table_->entry_num == 0) {
        this->set_service_data();        
    }

    if (serviceName.empty()) {
        std::cerr << "ERROR: serviceName is empty" << std::endl;
        return -1;
    }
    if (clientName.empty()) {
        std::cerr << "ERROR: clientName is empty" << std::endl;
        return -1;
    }
    int namelen = serviceName.length();
    if (namelen > HAKO_SERVICE_NAMELEN_MAX) {
        std::cerr << "ERROR: serviceName is too long" << std::endl;
        return -1;
    }
    namelen = clientName.length();
    if (namelen > HAKO_CLIENT_NAMELEN_MAX) {
        std::cerr << "ERROR: clientName is too long" << std::endl;
        return -1;
    }
    int service_id = this->get_service_id(serviceName);
    if (service_id < 0) {
        std::cerr << "ERROR: service not found: " << serviceName << std::endl;
        return -1;
    }
    if (this->is_exist_client_on_service(serviceName, clientName)) {
        std::cerr << "ERROR: client already exists" << std::endl;
        return -1;
    }
    HakoServiceEntryTye& service_entry = this->get_service_entry(serviceName);
    for (int i = 0; i < service_entry.maxClients; i++) {
        if (service_entry.clientChannelMap[i].enabled == true) {
            continue;
        }
        int recv_event_id = -1;
        int client_channel_id = service_entry.clientChannelMap[i].responseChannelId;
        bool ret = this->register_data_recv_event(serviceName, client_channel_id, nullptr, recv_event_id);
        if (ret == false) {
            std::cerr << "ERROR: Failed to register data receive event for service client" << std::endl;
            return -1;
        }
        service_entry.clientChannelMap[i].responseRecvEventId = recv_event_id;
        service_entry.clientChannelMap[i].enabled = true;
        memcpy(service_entry.clientChannelMap[i].clientName, clientName.c_str(), clientName.length());
        service_entry.clientChannelMap[i].clientName[clientName.length()] = '\0';
        std::cout << "INFO: register_data_recv_event() serviceName: "
            << serviceName << " channel_id: " << client_channel_id
            << " recv_event_id: " << recv_event_id << std::endl;
        client_id = i;
        std::cout << "INFO: client_id: " << client_id << " clientName: " << service_entry.clientChannelMap[i].clientName << std::endl;
        break;
    }
    if (client_id < 0) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return -1;
    }
    return service_id;
}

int hako::data::pro::HakoProData::put_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    (void)asset_id; // asset_id is not used in this function
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null on put_request()" << std::endl;
        return -1;
    }
    if (service_id < 0 || service_id >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_id is invalid on put_request()" << std::endl;
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
    char* serviceName = service_table_->entries[service_id].serviceName;
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(serviceName, channel_id);
    if (real_id < 0) {
        std::cerr << "ERROR: real_id is invalid: serviceName: " << serviceName << " channel_id: " << channel_id << std::endl;
        return -1;
    }
    bool write_result = false;
    write_result = master_data_->get_pdu_data()->write_pdu(real_id, packet, packet_len);
    if (write_result == false) {
        std::cerr << "ERROR: put_request() failed to write pdu: real_id = " << real_id <<" packet_len = " << packet_len << std::endl;
        return -1;
    }
    //std::cout << "INFO: put_request() success: real_id = " << real_id << " packet_len = " << packet_len << std::endl;
    return 0;
}

int hako::data::pro::HakoProData::get_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (service_table_ == nullptr) {
        std::cerr << "ERROR: service_table_ is null on get_response()" << std::endl;
        return -1;
    }
    if (service_id < 0 || service_id >= HAKO_SERVICE_MAX) {
        std::cerr << "ERROR: service_id is invalid on get_response()" << std::endl;
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
    auto ret = this->get_recv_event(asset_id, real_id, service_table_->entries[service_id].clientChannelMap[client_id].responseRecvEventId);
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
        std::cerr << "ERROR: get_response() failed to read pdu" << std::endl;
        return -1;
    }
    return 0;
}
