#include "hako_service_impl.hpp"
#include "hako_impl.hpp"
#include <fstream>
#include <iostream>

hako::service::impl::server::HakoServiceImplType hako_service_instance;

int hako::service::impl::initialize(const char* service_config_path, std::shared_ptr<hako::IHakoAssetController> hako_asset)
{
    hako_service_instance.is_initialized = false;
    std::ifstream ifs(service_config_path);
    
    if (!ifs.is_open()) {
        std::cerr << "Error: Failed to open service config file: " << service_config_path << std::endl;
        return -1;
    }

    try {
        ifs >> hako_service_instance.param;
        int pduMetaDataSize = hako_service_instance.param["pduMetaDataSize"];
        for (const auto& item : hako_service_instance.param["services"]) {
            hako::service::impl::Service s;
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
            bool ret = hako_asset->create_pdu_lchannel(s.name.c_str(), HAKO_SERVICE_SERVER_CHANNEL_ID, s.server_total_size);
            if (ret == false) {
                std::cerr << "Error: Failed to create PDU channel for service server: " << s.name << std::endl;
                return -1;
            }
            ret = hako_asset->create_pdu_lchannel(s.name.c_str(), HAKO_SERVICE_CLIENT_CHANNEL_ID, s.client_total_size);
            if (ret == false) {
                std::cerr << "Error: Failed to create PDU channel for service client: " << s.name << std::endl;
                return -1;
            }
            hako_service_instance.services.push_back(s);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse service config JSON: " << e.what() << std::endl;
        return -1;
    }
    ifs.close();
    hako_service_instance.is_initialized = true;
    return 0;
}

bool hako::service::impl::get_services(std::vector<hako::service::impl::Service>& services)
{
    if (!hako_service_instance.is_initialized) {
        return false;
    }
    services = hako_service_instance.services;
    return true;
}

/*
 * Service server API
 */
int hako::service::impl::server::create(const char* serviceName)
{
    if (!hako_service_instance.is_initialized) {
        std::cerr << "Error: not initialized." << std::endl;
        return -1;
    }
    if (serviceName == nullptr || *serviceName == '\0') {
        std::cerr << "Error: serviceName is not set." << std::endl;
        return -1;
    }
    int namelen = strlen(serviceName);
    if (namelen > HAKO_SERVICE_NAMELEN_MAX) {
        std::cerr << "ERROR: serviceName is too long" << std::endl;
        return -1;
    }
    bool found = false;
    for (const auto& service : hako_service_instance.services) {
        if (service.name == serviceName) {
            found = true;
            break;
        }
    }
    if (!found) {
        std::cerr << "ERROR: Service not found: " << serviceName << std::endl;
        return -1;
    }

    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    int service_id = -1;
    pro_data->lock_memory();
    {
        service_id = pro_data->create_service(serviceName);
    }
    pro_data->unlock_memory();
    if (service_id < 0) {
        std::cerr << "ERROR: service_id is invalid" << std::endl;
    }
    else {
        std::cout << "INFO: Service server created successfully: " << serviceName << std::endl;
    }
    return service_id;
}

int hako::service::impl::server::get_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (!hako_service_instance.is_initialized) {
        std::cerr << "Error: not initialized." << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len == 0) {
        std::cerr << "Error: packet is null or packet_len is 0." << std::endl;
        return -1;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    int ret = pro_data->get_request(asset_id, service_id, client_id, packet, packet_len);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int hako::service::impl::server::put_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (!hako_service_instance.is_initialized) {
        std::cerr << "Error: not initialized." << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len == 0) {
        std::cerr << "Error: packet is null or packet_len is 0." << std::endl;
        return -1;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    int ret = pro_data->put_response(asset_id, service_id, client_id, packet, packet_len);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

/*
 * Service client API
 */
int hako::service::impl::client::create(const char* serviceName, const char* clientName, int& client_id)
{
    if (!hako_service_instance.is_initialized) {
        std::cerr << "Error: not initialized." << std::endl;
        return -1;
    }
    if (serviceName == nullptr || *serviceName == '\0') {
        std::cerr << "Error: serviceName is not set." << std::endl;
        return -1;
    }
    if (clientName == nullptr || *clientName == '\0') {
        std::cerr << "Error: clientName is not set." << std::endl;
        return -1;
    }
    int namelen = strlen(serviceName);
    if (namelen > HAKO_SERVICE_NAMELEN_MAX) {
        std::cerr << "ERROR: serviceName is too long" << std::endl;
        return -1;
    }
    namelen = strlen(clientName);
    if (namelen > HAKO_CLIENT_NAMELEN_MAX) {
        std::cerr << "ERROR: clientName is too long" << std::endl;
        return -1;
    }
    bool found = false;
    for (const auto& service : hako_service_instance.services) {
        if (service.name == serviceName) {
            found = true;
            break;

        }
    }
    if (!found) {
        std::cerr << "ERROR: Service not found: " << serviceName << std::endl;
        return -1;
    }

    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    int service_id = -1;
    pro_data->lock_memory();
    {
        service_id = pro_data->create_service_client(serviceName, clientName, client_id);
    }
    pro_data->unlock_memory();
    if (service_id < 0) {
        std::cerr << "ERROR: service_id is invalid" << std::endl;
    }
    else {
        std::cout << "INFO: Service client created successfully: " << clientName << std::endl;
    }
    return service_id;
}

int hako::service::impl::client::put_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (!hako_service_instance.is_initialized) {
        std::cerr << "Error: not initialized." << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len == 0) {
        std::cerr << "Error: packet is null or packet_len is 0." << std::endl;
        return -1;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    int ret = pro_data->put_request(asset_id, service_id, client_id, packet, packet_len);
    if (ret < 0) {
        return -1;
    }
    return 0;
}
int hako::service::impl::client::get_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len)
{
    if (!hako_service_instance.is_initialized) {
        std::cerr << "Error: not initialized." << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len == 0) {
        std::cerr << "Error: packet is null or packet_len is 0." << std::endl;
        return -1;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    int ret = pro_data->get_response(asset_id, service_id, client_id, packet, packet_len);
    if (ret < 0) {
        return -1;
    }
    return 0;
}