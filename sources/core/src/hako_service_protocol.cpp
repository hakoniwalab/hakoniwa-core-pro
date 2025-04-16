#include "hako_service_protocol.hpp"

bool hako::service::HakoServiceServerProtocol::initialize(const char* serviceName, const char* assetName)
{
    if (server_ == nullptr) {
        return false;
    }
    server_->initialize(serviceName, assetName);
    return true;
}
hako::service::HakoServiceServerEventType hako::service::HakoServiceServerProtocol::run()
{
    auto event = HAKO_SERVICE_SERVER_EVENT_NONE;
    auto client_id = server_->get_current_client_id();
    HakoServiceServerStateType state = server_->get_state();
    switch (state) {
        case HAKO_SERVICE_SERVER_STATE_IDLE:
            if (server_->recv_request(client_id) != nullptr) {
                event = HAKO_SERVICE_SERVER_REQUEST_IN;
                if (!convertor_request_.pdu2cpp((char*)server_->get_request_buffer(), request_header_)) {
                    std::cerr << "ERROR: convertor.pdu2cpp() failed" << std::endl;
                    return HAKO_SERVICE_SERVER_EVENT_NONE;
                }
                //TODO header check
                server_->event_start_service(client_id);
            }
            break;
        case HAKO_SERVICE_SERVER_STATE_DOING:
            break;
        case HAKO_SERVICE_SERVER_STATE_CANCELING:
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
    return server_->get_request_buffer();
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