#include "hako_service_protocol.hpp"
#include "hako_impl.hpp"

bool hako::service::HakoServiceServerProtocol::initialize(const char* serviceName, const char* assetName)
{
    if (server_ == nullptr) {
        return false;
    }
    server_->initialize(serviceName, assetName);
    if (server_->get_request_pdu_size() <= 0 || server_->get_response_pdu_size() <= 0) {
        std::cerr << "ERROR: request_pdu_size_ or response_pdu_size_ is invalid" << std::endl;
        return false;
    }
    request_pdu_buffer_ = new char[server_->get_request_pdu_size()];
    response_pdu_buffer_ = new char[server_->get_response_pdu_size()];
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
        std::cerr << "ERROR: service_name is invalid" << std::endl;
        return false;
    }
    if (server_->is_exist_client(header.client_name) == false) {
        std::cerr << "ERROR: client_name is invalid" << std::endl;
        return false;
    }
    if (header.opcode >= HAKO_SERVICE_OPERATION_NUM) {
        std::cerr << "ERROR: opcode is invalid" << std::endl;
        return false;
    }
    return true;
}
bool hako::service::HakoServiceServerProtocol::copy_user_buffer(const HakoCpp_ServiceRequestHeader& header)
{
    char* src = (char*)server_->get_request_buffer();
    char* dst = (char*)request_pdu_buffer_;
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
    return request_pdu_buffer_;
}
bool hako::service::HakoServiceServerProtocol::set_response_header(HakoCpp_ServiceResponseHeader& header, HakoServiceStatusType status, HakoServiceResultCodeType result_code)
{
    header.request_id = request_header_.request_id;
    header.service_name = server_->get_service_name();
    header.client_name = server_->get_client_name(server_->get_current_client_id());
    header.status = status;
    header.processing_percentage = percentage_;
    header.result_code = result_code;
    return true;
}
bool hako::service::HakoServiceServerProtocol::reply(char* packet, int packet_len)
{
    auto ret = server_->send_response(server_->get_current_client_id(), packet, packet_len);
    if (ret) {
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

bool hako::service::HakoServiceServerProtocol::send_response(HakoServiceStatusType status, HakoServiceResultCodeType result_code)
{
    HakoCpp_ServiceResponseHeader header;
    set_response_header(header, status, result_code);
    int pdu_size = convertor_response_.cpp2pdu(header, (char*)server_->get_request_buffer(), server_->get_response_pdu_size());
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return false;
    }
    return reply((char*)server_->get_request_buffer(), server_->get_response_pdu_size());
}
