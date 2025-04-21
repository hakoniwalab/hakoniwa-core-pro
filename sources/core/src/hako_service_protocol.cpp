#include "hako_service_protocol.hpp"
#include "hako_pro.hpp"

bool hako::service::HakoServiceServerProtocol::initialize(const char* serviceName, const char* assetName)
{
    if (server_ == nullptr) {
        return false;
    }
    bool ret = server_->initialize(serviceName, assetName);
    if (!ret) {
        std::cerr << "ERROR: server_->initialize() failed" << std::endl;
        return false;
    }
    if (server_->get_request_pdu_size() <= 0 || server_->get_response_pdu_size() <= 0) {
        std::cerr << "ERROR: request_pdu_size_ or response_pdu_size_ is invalid" << std::endl;
        return false;
    }
    request_pdu_buffer_ = std::make_unique<char[]>(server_->get_request_pdu_size());
    response_pdu_buffer_ = std::make_unique<char[]>(server_->get_response_pdu_size());
    if (request_pdu_buffer_ == nullptr || response_pdu_buffer_ == nullptr) {
        std::cerr << "ERROR: request_pdu_buffer_ or response_pdu_buffer_ is null" << std::endl;
        return false;
    }
    return true;
}
/*
 * result:
 *  data_recv_in: true if data is received
 *  return value: true if received packet is valid or no data received
 */
bool hako::service::HakoServiceServerProtocol::recv_request(int client_id, HakoCpp_ServiceRequestHeader& header, bool& data_recv_in)
{
    data_recv_in = false;
    if (server_->recv_request(client_id) != nullptr) {
        // data received
        data_recv_in = true;
        if (!convertor_request_.pdu2cpp((char*)server_->get_request_buffer(), header)) {
            std::cerr << "ERROR: convertor.pdu2cpp() failed" << std::endl;
            return false;
        }
        if (!validate_header(header)) {
            std::cerr << "ERROR: header is invalid" << std::endl;
            return false;
        }
        return true;
    }
    // no data received
    return true;
}

bool hako::service::HakoServiceServerProtocol::validate_header(HakoCpp_ServiceRequestHeader& header)
{
    if (header.service_name != server_->get_service_name()) {
        std::cerr << "ERROR: service_name is invalid: " << header.service_name << std::endl;
        return false;
    }
    if (server_->is_exist_client(header.client_name) == false) {
        std::cerr << "ERROR: client_name is invalid: " << header.client_name << std::endl;
        return false;
    }
    if (header.opcode >= HAKO_SERVICE_OPERATION_NUM) {
        std::cerr << "ERROR: opcode is invalid: " << header.opcode << std::endl;
        return false;
    }
    return true;
}
bool hako::service::HakoServiceServerProtocol::copy_user_buffer(const HakoCpp_ServiceRequestHeader& header)
{
    char* src = (char*)server_->get_request_buffer();
    char* dst = (char*)request_pdu_buffer_.get();
    int src_len = server_->get_request_pdu_size();
    memcpy(dst, src, src_len);
    request_header_ = header;
    return true;
}
hako::service::HakoServiceServerEventType hako::service::HakoServiceServerProtocol::poll()
{
    HakoCpp_ServiceRequestHeader header;
    auto event = HAKO_SERVICE_SERVER_EVENT_NONE;
    auto client_id = server_->get_current_client_id();
    HakoServiceServerStateType state = server_->get_state();
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return HAKO_SERVICE_SERVER_EVENT_NONE;
    }

    bool data_recv_in = false;
    bool result = recv_request(client_id, header, data_recv_in);
    if (result == false) {
        std::cerr << "ERROR: recv_request() failed" << std::endl;
        if (send_response(HAKO_SERVICE_STATUS_ERROR, HAKO_SERVICE_RESULT_CODE_INVALID) == false) {
            std::cerr << "ERROR: send_response() failed" << std::endl;
        }
        return HAKO_SERVICE_SERVER_EVENT_NONE;
    }
    else if (data_recv_in == false) {
        if ((state == HAKO_SERVICE_SERVER_STATE_DOING) && (header.status_poll_interval_msec > 0)) {
            auto now = pro_data->get_world_time_usec();
            if ((last_poll_time_ == 0) || (now - last_poll_time_) > header.status_poll_interval_msec * 1000) {
                if (send_response(HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_OK) == false) {
                    std::cerr << "ERROR: send_response() failed" << std::endl;
                }
            }
            last_poll_time_ = now;
        }
        return HAKO_SERVICE_SERVER_EVENT_NONE;
    }

    switch (state) {
        case HAKO_SERVICE_SERVER_STATE_IDLE:
            if (header.opcode == HAKO_SERVICE_OPERATION_CODE_REQUEST) {
                event = HAKO_SERVICE_SERVER_REQUEST_IN;
                copy_user_buffer(header);
                server_->event_start_service(client_id);
                percentage_ = 0;
            }
            else {
                if (send_response(HAKO_SERVICE_STATUS_ERROR, HAKO_SERVICE_RESULT_CODE_INVALID) == false) {
                    std::cerr << "ERROR: send_response() failed" << std::endl;
                }
            }
            break;
        case HAKO_SERVICE_SERVER_STATE_DOING:
            if (header.opcode == HAKO_SERVICE_OPERATION_CODE_CANCEL) {
                event = HAKO_SERVICE_SERVER_REQUEST_CANCEL;
                server_->event_cancel_service(client_id);
            }
            else {
                if (send_response(HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_BUSY) == false) {
                    std::cerr << "ERROR: send_response() failed" << std::endl;
                }
            }
            break;
        case HAKO_SERVICE_SERVER_STATE_CANCELING:
            if (send_response(HAKO_SERVICE_STATUS_CANCELING, HAKO_SERVICE_RESULT_CODE_BUSY) == false) {
                std::cerr << "ERROR: send_response() failed" << std::endl;
            }
            break;
        default:
            break;
    }
    return event;
}

void* hako::service::HakoServiceServerProtocol::get_request()
{
    if (server_->get_state() != HAKO_SERVICE_SERVER_STATE_DOING) {
        return nullptr;
    }
    return request_pdu_buffer_.get();
}
bool hako::service::HakoServiceServerProtocol::set_response_header(HakoCpp_ServiceResponseHeader& header, HakoServiceStatusType status, HakoServiceResultCodeType result_code)
{
    header.request_id = request_header_.request_id;
    header.service_name = server_->get_service_name();
    header.client_name = server_->get_client_name(server_->get_current_client_id());
    header.status = status;
    header.processing_percentage = percentage_;
    header.result_code = result_code;
    //debug
    //std::cout << "INFO: set_response_header() request_id=" << header.request_id << std::endl;
    //std::cout << "INFO: set_response_header() service_name=" << header.service_name << std::endl;
    //std::cout << "INFO: set_response_header() client_name=" << header.client_name << std::endl;
    //std::cout << "INFO: set_response_header() status=" << (int)header.status << std::endl;
    //std::cout << "INFO: set_response_header() processing_percentage=" << (int)header.processing_percentage << std::endl;
    //std::cout << "INFO: set_response_header() result_code=" << (int)header.result_code << std::endl;
    return true;
}
bool hako::service::HakoServiceServerProtocol::reply(char* packet, int packet_len)
{
    auto ret = server_->send_response(server_->get_current_client_id(), packet, packet_len);
    if (ret) {
        std::cout << "INFO: reply() success" << std::endl;
        server_->event_done_service(server_->get_current_client_id());
    }
    server_->next_client();
    return ret;
}
void hako::service::HakoServiceServerProtocol::cancel_done()
{
    server_->event_done_service(server_->get_current_client_id());
    server_->next_client();
}

void* hako::service::HakoServiceServerProtocol::get_response_buffer(HakoServiceStatusType status, HakoServiceResultCodeType result_code)
{
    HakoCpp_ServiceResponseHeader header;
    //std::cout << "INFO: get_response_buffer() called" << std::endl;
    //std::cout << "INFO: get_response_buffer() status=" << (int)status << std::endl;
    //std::cout << "INFO: get_response_buffer() result_code=" << (int)result_code << std::endl;
    set_response_header(header, status, result_code);
    int pdu_size = convertor_response_.cpp2pdu(header, (char*)response_pdu_buffer_.get(), server_->get_response_pdu_size());
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return nullptr;
    }
    return response_pdu_buffer_.get();
}
bool hako::service::HakoServiceServerProtocol::send_response(HakoServiceStatusType status, HakoServiceResultCodeType result_code)
{
    HakoCpp_ServiceResponseHeader header;
    set_response_header(header, status, result_code);
    int pdu_size = convertor_response_.cpp2pdu(header, (char*)server_->get_response_buffer(), server_->get_response_pdu_size());
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return false;
    }
    return reply((char*)server_->get_response_buffer(), server_->get_response_pdu_size());
}
/*
 * HakoServiceClientProtocol
 */
bool hako::service::HakoServiceClientProtocol::initialize(const char* serviceName, const char* clientName, const char* assetName)
{
    if (client_ == nullptr) {
        return false;
    }
    bool ret = client_->initialize(serviceName, clientName, assetName);
    if (ret == false) {
        std::cerr << "ERROR: client_->initialize() failed" << std::endl;
        return false;
    }
    if (client_->get_request_pdu_size() <= 0 || client_->get_response_pdu_size() <= 0) {
        std::cerr << "ERROR: request_pdu_size_ or response_pdu_size_ is invalid" << std::endl;
        return false;
    }
    request_pdu_buffer_ = std::make_unique<char[]>(client_->get_request_pdu_size());
    response_pdu_buffer_ = std::make_unique<char[]>(client_->get_response_pdu_size());    
    if (request_pdu_buffer_ == nullptr || response_pdu_buffer_ == nullptr) {
        std::cerr << "ERROR: request_pdu_buffer_ or response_pdu_buffer_ is null" << std::endl;
        return false;
    }
    return true;
}
bool hako::service::HakoServiceClientProtocol::recv_response(HakoCpp_ServiceResponseHeader& header, bool& data_recv_in)
{
    data_recv_in = false;
    if (client_->recv_response() != nullptr) {
        // data received
        data_recv_in = true;
        if (!convertor_response_.pdu2cpp((char*)client_->get_response_buffer(), header)) {
            std::cerr << "ERROR: convertor.pdu2cpp() failed" << std::endl;
            return false;
        }
        if (!validate_header(header)) {
            std::cerr << "ERROR: header is invalid" << std::endl;
            return false;
        }
        return true;
    }
    // no data received
    return true;
}
bool hako::service::HakoServiceClientProtocol::validate_header(HakoCpp_ServiceResponseHeader& header)
{
    if (header.service_name != client_->get_service_name()) {
        std::cerr << "ERROR: service_name is invalid: " << header.service_name << std::endl;
        return false;
    }
    if (header.client_name != client_->get_client_name()) {
        std::cerr << "ERROR: client_name is invalid: " << header.client_name << std::endl;
        return false;
    }
    if (header.result_code >= HAKO_SERVICE_RESULT_CODE_NUM) {
        std::cerr << "ERROR: result_code is invalid: " << header.result_code << std::endl;
        return false;
    }
    return true;
}
bool hako::service::HakoServiceClientProtocol::copy_user_buffer(const HakoCpp_ServiceResponseHeader& header)
{
    char* src = (char*)client_->get_response_buffer();
    char* dst = (char*)response_pdu_buffer_.get();
    int src_len = client_->get_response_pdu_size();
    memcpy(dst, src, src_len);
    response_header_ = header;
    return true;
}
hako::service::HakoServiceClientEventType hako::service::HakoServiceClientProtocol::poll()
{
    HakoCpp_ServiceResponseHeader header;
    auto event = HAKO_SERVICE_CLIENT_EVENT_NONE;
    auto state = client_->get_state();
    if (state == HAKO_SERVICE_CLIENT_STATE_IDLE) {
        return HAKO_SERVICE_CLIENT_EVENT_NONE;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return HAKO_SERVICE_CLIENT_EVENT_NONE;
    }
    bool data_recv_in = false;
    bool result = recv_response(header, data_recv_in);
    if (result == false) {
        std::cerr << "ERROR: recv_response() failed" << std::endl;
        return HAKO_SERVICE_CLIENT_EVENT_NONE;
    }
    else if (data_recv_in == false) {
        return HAKO_SERVICE_CLIENT_EVENT_NONE;
    }
    std::cout << "INFO: data_recv_in: status = " << (int)header.status << std::endl;
    event = HAKO_SERVICE_CLIENT_EVENT_NONE;
    switch (header.status)
    {
        case HAKO_SERVICE_STATUS_DONE:
            event = HAKO_SERVICE_CLIENT_RESPONSE_IN;
            copy_user_buffer(header);
            client_->event_done_service();
            request_id_++;
            break;
        case HAKO_SERVICE_STATUS_ERROR:
            event = HAKO_SERVICE_CLIENT_RESPONSE_IN;
            std::cerr << "ERROR: service error" << std::endl;
            copy_user_buffer(header);
            client_->event_done_service();
            request_id_++;
            break;
        case HAKO_SERVICE_STATUS_DOING:
            //TIMEOUT check
            if (request_header_.status_poll_interval_msec > 0) {
                auto now = pro_data->get_world_time_usec();
                if ((last_poll_time_ == 0) || (now - last_poll_time_) > request_header_.status_poll_interval_msec * 1000) {
                    event = HAKO_SERVICE_CLIENT_REQUEST_TIMEOUT;
                    std::cerr << "ERROR: request timeout" << std::endl;
                    cancel_request();
                }
                last_poll_time_ = now;
            }
            break;
        default:
            //HAKO_SERVICE_STATUS_NONE
            //HAKO_SERVICE_STATUS_CANCELING
            //nothing to do
            break;
    }

    return event;
}
bool hako::service::HakoServiceClientProtocol::set_request_header(HakoCpp_ServiceRequestHeader& header, HakoServiceOperationCodeType opcode, int poll_interval_msec)
{
    header.request_id = request_id_;
    header.service_name = client_->get_service_name();
    header.client_name = client_->get_client_name();
    //std::cout << "INFO: client_name=" << header.client_name << std::endl;
    //std::cout << "INFO: request_id=" << header.request_id << std::endl;
    //std::cout << "INFO: service_name=" << header.service_name << std::endl;
    header.opcode = opcode;
    header.status_poll_interval_msec = poll_interval_msec;
    return true;
}
void* hako::service::HakoServiceClientProtocol::get_request_buffer(int opcode, int poll_interval_msec)
{
    HakoCpp_ServiceRequestHeader header;
    set_request_header(header, (HakoServiceOperationCodeType)opcode, poll_interval_msec);
    int pdu_size = convertor_request_.cpp2pdu(header, (char*)this->request_pdu_buffer_.get(), client_->get_request_pdu_size());
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return nullptr;
    }
    return this->request_pdu_buffer_.get();
}

bool hako::service::HakoServiceClientProtocol::send_request(HakoServiceOperationCodeType opcode, int poll_interval_msec)
{
    HakoCpp_ServiceRequestHeader header;
    set_request_header(header, opcode, poll_interval_msec);
    int pdu_size = convertor_request_.cpp2pdu(header, (char*)client_->get_request_buffer(), client_->get_request_pdu_size());
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return false;
    }
    return client_->send_request((char*)client_->get_request_buffer(), client_->get_request_pdu_size());
}
void* hako::service::HakoServiceClientProtocol::get_response()
{
    if (client_->get_state() != HAKO_SERVICE_CLIENT_STATE_DOING) {
        return nullptr;
    }
    return response_pdu_buffer_.get();
}
bool hako::service::HakoServiceClientProtocol::request(char* packet, int packet_len)
{
    std::cout << "INFO: request() packet_len=" << packet_len << std::endl;
    HakoCpp_ServiceRequestHeader header;
    int pdu_size = convertor_request_.pdu2cpp(packet, header);
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return false;
    }
    auto ret = client_->send_request(packet, packet_len);
    if (ret) {
        request_header_ = header;
        client_->event_start_service();
    }
    std::cout << "INFO: request_header_.request_id=" << request_header_.request_id << std::endl;
    return ret;
}
void hako::service::HakoServiceClientProtocol::cancel_request()
{
    if (client_->get_state() != HAKO_SERVICE_CLIENT_STATE_DOING) {
        return;
    }
    //send cancel request
    auto ret = send_request(HAKO_SERVICE_OPERATION_CODE_CANCEL, 0);
    if (ret == false) {
        std::cerr << "ERROR: send_request() failed" << std::endl;
        return;
    }
    client_->event_cancel_service();
}
