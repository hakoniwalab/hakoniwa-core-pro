#include "hako_service_impl_client.hpp"
#include "hako_pro.hpp"
#include <fstream>
#include <iostream>

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
    //pro_data->lock_memory();
    service_id_ = pro_data->create_service_client(serviceName, clientName, client_id_);
    //pro_data->unlock_memory();
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
        (char*)get_temp_response_buffer(), get_response_pdu_size());
    if (ret < 0) {
        return nullptr;
    }
    return (char*)get_temp_response_buffer();
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
    //std::cout << "INFO: HakoServiceClient::event_start_service() called" << std::endl;
    if (state_ != HAKO_SERVICE_CLIENT_STATE_IDLE) {
        std::cerr << "ERROR: service is not idle: state = " << state_ << std::endl;
        return false;
    }
    state_ = HAKO_SERVICE_CLIENT_STATE_DOING;
    //std::cout << "INFO: Service client started successfully" << std::endl;
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