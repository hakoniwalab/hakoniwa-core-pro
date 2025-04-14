#include "hako_service_impl.hpp"
#include "hako_impl.hpp"
#include <fstream>
#include <iostream>

hako::service::impl::HakoServiceImplType hako_service_instance;

int hako::service::impl::initialize(const char* service_config_path)
{
    hako_service_instance.is_initialized = true;
    if (service_config_path == nullptr || *service_config_path == '\0') {
        std::cerr << "Error: service_config_path is not set." << std::endl;
        return -1;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    if (!pro_data->initialize_service(service_config_path)) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): initialize_service failed" << std::endl;
        return -1;
    }
    std::cout << "INFO: Service initialized successfully: " << service_config_path << std::endl;
    return 0;
}

/*
 * Service server API
 */
void hako::service::impl::HakoServiceServer::initialize(const char* serviceName, const char* assetName) 
{
    if (serviceName == nullptr || *serviceName == '\0') {
        throw std::runtime_error("Error: serviceName is not set.");
    }
    if (assetName != nullptr && *assetName == '\0') {
        throw std::runtime_error("Error: assetName is not set.");
    }
    if (assetName != nullptr) {
        int namelen = strlen(assetName);
        if (namelen > HAKO_FIXED_STRLEN_MAX) {
            throw std::runtime_error("ERROR: assetName is too long");
        }
    }
    int namelen = strlen(serviceName);
    if (namelen > HAKO_SERVICE_NAMELEN_MAX) {
        throw std::runtime_error("ERROR: serviceName is too long");
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        throw std::runtime_error("ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null");
    }
    if (assetName != nullptr) {
        asset_id_ = pro_data->get_asset_id(assetName);
    }
    else {
        asset_id_ = -1; //external
    }
    pro_data->lock_memory();
    service_id_ = pro_data->create_service(serviceName);
    pro_data->unlock_memory();
    if (service_id_ < 0) {
        throw std::runtime_error("ERROR: service_id is invalid");
    }
    service_name_ = serviceName;
    asset_name_ = assetName;
    auto& service_entry = pro_data->get_service_entry(service_name_);
    request_pdu_size_ = service_entry.pdu_size_request;
    response_pdu_size_ = service_entry.pdu_size_response;
    max_clients_ = service_entry.maxClients;
    request_pdu_buffer_ = new char[request_pdu_size_];
    response_pdu_buffer_ = new char[response_pdu_size_];
    if (request_pdu_buffer_ == nullptr || response_pdu_buffer_ == nullptr) {
        throw std::runtime_error("ERROR: request_pdu_buffer_ or response_pdu_buffer_ is null");
    }
    return;
}

/*
 * Service server API
 */
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