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
bool hako::service::impl::HakoServiceServer::initialize(const char* serviceName, const char* assetName) 
{
    std::cout << "INFO: HakoServiceServer::initialize() called" << std::endl;
    if (serviceName == nullptr || *serviceName == '\0') {
        std::cerr << "Error: serviceName is not set." << std::endl;
        return false;
    }
    if (assetName != nullptr && *assetName == '\0') {
        std::cerr << "Error: assetName is empty." << std::endl;
        return false;
    }
    if (assetName != nullptr) {
        int namelen = strlen(assetName);
        if (namelen > HAKO_FIXED_STRLEN_MAX) {
            std::cerr << "Error: assetName is too long" << std::endl;
            return false;
        }
    }
    int namelen = strlen(serviceName);
    if (namelen > HAKO_SERVICE_NAMELEN_MAX) {
        std::cerr << "Error: serviceName is too long" << std::endl;
        return false;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return false;
    }
    if (assetName != nullptr) {
        asset_id_ = pro_data->get_asset_id(assetName);
        asset_name_ = assetName;
    }
    else {
        asset_id_ = -1; //external
    }
    std::cout << "INFO: asset_id_: " << asset_id_ << std::endl;
    pro_data->lock_memory();
    service_id_ = pro_data->create_service(serviceName);
    pro_data->unlock_memory();
    if (service_id_ < 0) {
        std::cerr << "ERROR: service_id_ is invalid on HakoServiceServer::initialize()" << std::endl;
        return false;
    }
    service_name_ = serviceName;
    auto& service_entry = pro_data->get_service_entry(service_name_);
    request_pdu_size_ = service_entry.pdu_size_request;
    response_pdu_size_ = service_entry.pdu_size_response;
    max_clients_ = service_entry.maxClients;
    request_pdu_buffer_ = std::make_unique<char[]>(request_pdu_size_);
    response_pdu_buffer_ = std::make_unique<char[]>(response_pdu_size_);
    if (request_pdu_buffer_ == nullptr || response_pdu_buffer_ == nullptr) {
        std::cerr << "ERROR: request_pdu_buffer_ or response_pdu_buffer_ is null" << std::endl;
        return false;
    }
    state_.resize(max_clients_);
    for (int i = 0; i < max_clients_; ++i) {
        state_[i] = HAKO_SERVICE_SERVER_STATE_IDLE;
    }
    std::cout << "INFO: Service server initialized successfully: " << serviceName << std::endl;
    return true;
}
std::string hako::service::impl::HakoServiceServer::get_client_name(int client_id)
{
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return "";
    }
    return pro_data->get_client_name(service_id_, client_id);
}

char* hako::service::impl::HakoServiceServer::recv_request(int client_id)
{
    if (client_id < 0 || client_id >= max_clients_) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return nullptr;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return nullptr;
    }
    int ret = pro_data->get_request(asset_id_, service_id_, client_id, 
        get_request_pdu_buffer(), get_request_pdu_size());
    if (ret < 0) {
        return nullptr;
    }
    return get_request_pdu_buffer();
}
bool hako::service::impl::HakoServiceServer::send_response(int client_id, void* packet, int packet_len)
{
    if (client_id < 0 || client_id >= max_clients_) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return false;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return false;
    }
    int ret = pro_data->put_response(asset_id_, service_id_, client_id, 
        (char*)packet, packet_len);
    if (ret < 0) {
        return false;
    }
    return true;
}
bool hako::service::impl::HakoServiceServer::event_start_service(int client_id)
{
    if (client_id < 0 || client_id >= max_clients_) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return false;
    }
    if (state_[client_id] != HAKO_SERVICE_SERVER_STATE_IDLE) {
        std::cerr << "ERROR: service is not idle" << std::endl;
        return false;
    }
    state_[client_id] = HAKO_SERVICE_SERVER_STATE_DOING;
    return true;
}
bool hako::service::impl::HakoServiceServer::event_done_service(int client_id)
{
    if (client_id < 0 || client_id >= max_clients_) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return false;
    }
    if ((state_[client_id] != HAKO_SERVICE_SERVER_STATE_DOING && state_[client_id] != HAKO_SERVICE_SERVER_STATE_CANCELING)) {
        std::cerr << "ERROR: service is not doing" << std::endl;
        return false;
    }
    state_[client_id] = HAKO_SERVICE_SERVER_STATE_IDLE;
    return true;
}
bool hako::service::impl::HakoServiceServer::event_cancel_service(int client_id)
{
    if (client_id < 0 || client_id >= max_clients_) {
        std::cerr << "ERROR: client_id is invalid" << std::endl;
        return false;
    }
    if (state_[client_id] != HAKO_SERVICE_SERVER_STATE_DOING) {
        std::cerr << "ERROR: service is not doing" << std::endl;
        return false;
    }
    state_[client_id] = HAKO_SERVICE_SERVER_STATE_CANCELING;
    return true;
}
bool hako::service::impl::HakoServiceServer::is_exist_client(std::string client_name)
{
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return false;
    }
    return pro_data->is_exist_client_on_service(service_name_, client_name);
}


/*
 * Service client API
 */
bool hako::service::impl::HakoServiceClient::initialize(const char* serviceName, const char* clientName, const char* assetName)
{
    if (serviceName == nullptr || *serviceName == '\0') {
        std::cerr << "Error: serviceName is not set." << std::endl;
        return false;
    }
    if (clientName == nullptr || *clientName == '\0') {
        std::cerr << "Error: clientName is not set." << std::endl;
        return false;
    }
    int namelen = strlen(serviceName);
    if (namelen > HAKO_SERVICE_NAMELEN_MAX) {
        std::cerr << "Error: serviceName is too long" << std::endl;
        return false;
    }
    namelen = strlen(clientName);
    if (namelen > HAKO_CLIENT_NAMELEN_MAX) {
        std::cerr << "Error: clientName is too long" << std::endl;
        return false;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: HakoServiceClient::initialize(): pro_data is null" << std::endl;
        return false;
    }
    if (assetName != nullptr) {
        asset_id_ = pro_data->get_asset_id(assetName);
    }
    else {
        asset_id_ = -1; //external
    }
    pro_data->lock_memory();
    service_id_ = pro_data->create_service_client(serviceName, clientName, client_id_);
    pro_data->unlock_memory();
    if (service_id_ < 0) {
        std::cerr << "ERROR: service_id_ is invalid on HakoServiceClient::initialize()" << std::endl;
        return false;
    }
    service_name_ = serviceName;    
    auto& service_entry = pro_data->get_service_entry(service_name_);
    request_pdu_size_ = service_entry.pdu_size_request;
    response_pdu_size_ = service_entry.pdu_size_response;
    request_pdu_buffer_ = std::make_unique<char[]>(request_pdu_size_);
    response_pdu_buffer_ = std::make_unique<char[]>(response_pdu_size_);
    if (request_pdu_buffer_ == nullptr || response_pdu_buffer_ == nullptr) {
        std::cerr << "ERROR: request_pdu_buffer_ or response_pdu_buffer_ is null" << std::endl;
        return false;
    }
    client_name_ = clientName;
    return true;
}
char* hako::service::impl::HakoServiceClient::recv_response()
{
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return nullptr;
    }
    int ret = pro_data->get_response(asset_id_, service_id_, client_id_, 
        (char*)get_response_buffer(), get_response_pdu_size());
    if (ret < 0) {
        return nullptr;
    }
    return (char*)get_response_buffer();
}

bool hako::service::impl::HakoServiceClient::send_request(void* packet, int packet_len)
{
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return false;
    }

    int ret = pro_data->put_request(asset_id_, service_id_, client_id_, (char*)packet, packet_len);
    if (ret < 0) {
        return false;
    }
    return true;
}


bool hako::service::impl::HakoServiceClient::event_start_service()
{
    std::cout << "INFO: HakoServiceClient::event_start_service() called" << std::endl;
    if (state_ != HAKO_SERVICE_CLIENT_STATE_IDLE) {
        std::cerr << "ERROR: service is not idle: state = " << state_ << std::endl;
        return false;
    }
    state_ = HAKO_SERVICE_CLIENT_STATE_DOING;
    std::cout << "INFO: Service client started successfully" << std::endl;
    return true;
}
bool hako::service::impl::HakoServiceClient::event_done_service()
{
    if ((state_ != HAKO_SERVICE_CLIENT_STATE_DOING) && (state_ != HAKO_SERVICE_CLIENT_STATE_CANCELING)) {
        std::cerr << "ERROR: service is not doing" << std::endl;
        return false;
    }
    state_ = HAKO_SERVICE_CLIENT_STATE_IDLE;
    return true;
}
bool hako::service::impl::HakoServiceClient::event_cancel_service()
{
    if (state_ != HAKO_SERVICE_CLIENT_STATE_DOING) {
        std::cerr << "ERROR: service is not doing" << std::endl;
        return false;
    }
    state_ = HAKO_SERVICE_CLIENT_STATE_CANCELING;
    return true;
}