#include "hako_pro.hpp"
#include "hako_asset_service.h"
#include "hako_service_protocol.hpp"
#include "hako_service_impl.hpp"
#include <vector>
#include <unordered_map>

static std::unordered_map<int, std::pair<int, std::shared_ptr<hako::service::HakoServiceClientProtocol>>> service_clients;

int hako_asset_service_client_create(const char* assetName, const char* serviceName, const char* clientName, HakoServiceHandleType* handle)
{
    if (assetName == nullptr || serviceName == nullptr || clientName == nullptr || handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_create(): serviceName or clientName or handle is null" << std::endl;
        return -1;
    }
    auto client_impl = std::make_shared<hako::service::impl::HakoServiceClient>();
    if (!client_impl) {
        std::cerr << "ERROR: hako_asset_service_client_create(): client_impl is null" << std::endl;
        return -1;
    }
    auto client_protocol = std::make_shared<hako::service::HakoServiceClientProtocol>(client_impl);
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_create(): client_protocol is null" << std::endl;
        return -1;
    }
    bool ret = client_protocol->initialize(serviceName, clientName, assetName);
    if (!ret) {
        std::cerr << "ERROR: hako_asset_service_client_create(): client_protocol->initialize() failed" << std::endl;
        return -1;
    }
    int service_id = client_protocol->get_service_id();
    if (service_id < 0) {
        std::cerr << "ERROR: hako_asset_service_client_create(): service_id is invalid" << std::endl;
        return -1;
    }
    int client_id = client_protocol->get_client_id();
    service_clients[client_id] = std::make_pair(service_id, client_protocol);
    handle->service_id = service_id;
    handle->client_id = client_protocol->get_client_id();
    return 0;
}

int hako_asset_service_client_poll(const HakoServiceHandleType* handle)
{
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_poll(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_poll(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_poll(): client_protocol is null" << std::endl;
        return -1;
    }
    auto event = client_protocol->poll();
    switch (event) {
        case hako::service::HAKO_SERVICE_CLIENT_RESPONSE_IN:
            return HAKO_SERVICE_CLIENT_API_RESPONSE_IN;
        case hako::service::HAKO_SERVICE_CLIENT_REQUEST_CANCEL_DONE:
            return HAKO_SERVICE_CLIENT_API_REQUEST_CANCEL_DONE;
        case hako::service::HAKO_SERVICE_CLIENT_REQUEST_TIMEOUT:
            return HAKO_SERVICE_CLIENT_API_REQUEST_TIMEOUT;
        default:
            break;
    }
    return HAKO_SERVICE_CLIENT_API_EVENT_NONE;
}
int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t *packet_len, int opcode, int poll_interval_msec)
{
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_get_request_buffer(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_get_request_buffer(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_get_request_buffer(): client_protocol is null" << std::endl;
        return -1;
    }
    char* request_buffer = (char*)client_protocol->get_request_buffer(opcode, poll_interval_msec);
    if (request_buffer == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_get_request_buffer(): request_buffer is null" << std::endl;
        return -1;
    }
    *packet = request_buffer;
    *packet_len = client_protocol->get_request_pdu_size();
    return 0;
}

int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char** packet, size_t *packet_len)
{
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): client_protocol is null" << std::endl;
        return -1;
    }
    char* response_buffer = (char*)client_protocol->get_response();
    if (response_buffer == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): response_buffer is null" << std::endl;
        return -1;
    }
    *packet = response_buffer;
    *packet_len = client_protocol->get_response_pdu_size();
    return 0;
}

int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char *packet, size_t packet_len, int timeout)
{
    (void)timeout; // TODO
    //std::cout << "INFO: hako_asset_service_client_call_request(): handle: " << handle->client_id << std::endl;
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_call_request(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_call_request(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_call_request(): client_protocol is null" << std::endl;
        return -1;
    }
    bool ret = client_protocol->request(packet, packet_len);
    if (!ret) {
        std::cerr << "ERROR: hako_asset_service_client_call_request(): request() failed" << std::endl;
        return -1;
    }
    return 0;
}

int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char **packet, size_t *packet_len, int timeout)
{
    (void)timeout; // TODO
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_get_response(): client_protocol is null" << std::endl;
        return -1;
    }
    *packet = (char*)client_protocol->get_response_buffer();
    *packet_len = client_protocol->get_response_pdu_size();

    return 0;
}
int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle)
{
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_cancel_request(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_cancel_request(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_cancel_request(): client_protocol is null" << std::endl;
        return -1;
    }
    client_protocol->cancel_request();
    return 0;
}
int hako_asset_service_client_get_progress(const HakoServiceHandleType* handle)
{
    if (handle == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_get_progress(): handle is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_get_progress(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_get_progress(): client_protocol is null" << std::endl;
        return -1;
    }
    return client_protocol->get_progress();
}

int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status)
{
    if (handle == nullptr || status == nullptr) {
        std::cerr << "ERROR: hako_asset_service_client_status(): handle or status is null" << std::endl;
        return -1;
    }
    auto it = service_clients.find(handle->client_id);
    if (it == service_clients.end()) {
        std::cerr << "ERROR: hako_asset_service_client_status(): client not found" << std::endl;
        return -1;
    }
    auto client_protocol = it->second.second;
    if (!client_protocol) {
        std::cerr << "ERROR: hako_asset_service_client_status(): client_protocol is null" << std::endl;
        return -1;
    }
    switch (client_protocol->state()) {
        case hako::service::HAKO_SERVICE_CLIENT_STATE_IDLE:
            *status = HAKO_SERVICE_CLIENT_API_STATE_IDLE;
            break;
        case hako::service::HAKO_SERVICE_CLIENT_STATE_DOING:
            *status = HAKO_SERVICE_CLIENT_API_STATE_DOING;
            break;
        case hako::service::HAKO_SERVICE_CLIENT_STATE_CANCELING:
            *status = HAKO_SERVICE_CLIENT_API_STATE_CANCELING;
            break;
        default:
            std::cerr << "ERROR: hako_asset_service_client_status(): invalid state" << std::endl;
            return -1;
    }
    return 0;
}