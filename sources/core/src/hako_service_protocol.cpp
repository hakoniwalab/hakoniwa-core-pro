#include "hako_service_protocol.hpp"

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
bool hako::service::HakoServiceServerProtocol::recv_request(int client_id, HakoCpp_ServiceRequestHeader& header, bool& data_recv_in)
{
    data_recv_in = false;
    if (server_->recv_request(client_id) != nullptr) {
        data_recv_in = true;
        if (!convertor_request_.pdu2cpp((char*)server_->get_request_buffer(), header)) {
            std::cerr << "ERROR: convertor.pdu2cpp() failed" << std::endl;
            return false;
        }
        if (!validate_header(request_header_)) {
            std::cerr << "ERROR: header is invalid" << std::endl;
            return false;
        }
        return true;
    }
    return false;
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

    bool data_recv_in = false;
    bool rcv_result = recv_request(client_id, header, data_recv_in);
    if (data_recv_in == false) {
        return HAKO_SERVICE_SERVER_EVENT_NONE;
    }
    if (rcv_result == false) {
        std::cerr << "ERROR: recv_request() failed" << std::endl;
        //TODO error reply
        return HAKO_SERVICE_SERVER_EVENT_NONE;
    }
    switch (state) {
        case HAKO_SERVICE_SERVER_STATE_IDLE:
            if (header.opcode == HAKO_SERVICE_OPERATION_CODE_REQUEST) {
                event = HAKO_SERVICE_SERVER_REQUEST_IN;
                copy_user_buffer(header);
                server_->event_start_service(client_id);
            }
            else {
                //TODO error reply
            }
            break;
        case HAKO_SERVICE_SERVER_STATE_DOING:
            if (header.opcode == HAKO_SERVICE_OPERATION_CODE_CANCEL) {
                event = HAKO_SERVICE_SERVER_REQUEST_CANCEL;
                server_->event_cancel_service(client_id);
            }
            else {
                //TODO error reply
            }
            break;
        case HAKO_SERVICE_SERVER_STATE_CANCELING:
            //TODO error reply
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
bool hako::service::HakoServiceServerProtocol::put_reply(void* packet, int packet_len, int result_code)
{
    if (server_->get_state() != HAKO_SERVICE_SERVER_STATE_DOING) {
        return false;
    }
    //TODO set response header
    response_header_.result_code = result_code;
    int pdu_size = convertor_response_.cpp2pdu(response_header_, (char*)packet, packet_len);
    if (pdu_size < 0) {
        std::cerr << "ERROR: convertor.cpp2pdu() failed" << std::endl;
        return false;
    }
    auto ret = server_->send_response(server_->get_current_client_id(), packet, packet_len);
    if (ret) {
        server_->event_done_service(server_->get_current_client_id());
    }
    return ret;
}