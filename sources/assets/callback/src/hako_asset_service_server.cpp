#include "hako_pro.hpp"
#include "hako_asset_service.h"
#include "hako_service_impl_server.hpp"
#include "hako_service_protocol_server.hpp"
#include <vector>
#include <unordered_map>


static std::unordered_map<int, std::shared_ptr<hako::service::HakoServiceServerProtocol>> service_servers;

int hako_asset_service_server_create(const char* assetName, const char* serviceName)
{
    //std::cout << "INFO: hako_asset_service_server_create(): assetName: " << assetName << ", serviceName: " << serviceName << std::endl;
    for (const auto& [id, server] : service_servers) {
        if (server->get_service_name() == serviceName) {
            std::cout << "WARN: hako_asset_service_server_create(): service already exists" << std::endl;
            return id;
        }
    }
    //std::cout << "INFO: hako_asset_service_server_create(): creating new service server" << std::endl;
    auto server_impl = std::make_shared<hako::service::impl::HakoServiceServer>();
    if (!server_impl) {
        std::cerr << "ERROR: hako_asset_service_server_create(): server_impl is null" << std::endl;
        return -1;
    }
    //std::cout << "INFO: hako_asset_service_server_create(): server_impl created" << std::endl;
    auto server_protocol = std::make_shared<hako::service::HakoServiceServerProtocol>(server_impl);
    if (!server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_create(): server_protocol is null" << std::endl;
        return -1;
    }
    //std::cout << "INFO: hako_asset_service_server_create(): server_protocol created" << std::endl;
    bool ret = server_protocol->initialize(serviceName, assetName);
    if (!ret) {
        std::cerr << "ERROR: hako_asset_service_server_create(): server_protocol->initialize() failed" << std::endl;
        return -1;
    }
    //std::cout << "INFO: hako_asset_service_server_create(): server_protocol initialized" << std::endl;
    int service_id = server_protocol->get_service_id();
    if (service_id < 0) {
        std::cerr << "ERROR: hako_asset_service_server_create(): service_id is invalid" << std::endl;
        return -1;
    }
    //std::cout << "INFO: hako_asset_service_server_create(): service_id: " << service_id << std::endl;
    service_servers[service_id] = server_protocol;
    return service_id;
}

int hako_asset_service_server_poll(int service_id)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_get_request(): service_server_protocol is null" << std::endl;
        return -1;
    }
    hako::service::HakoServiceServerEventType event = service_server_protocol->poll();
    switch (event) {
        case hako::service::HAKO_SERVICE_SERVER_REQUEST_IN:
            return HAKO_SERVICE_SERVER_API_REQUEST_IN;
        case hako::service::HAKO_SERVICE_SERVER_REQUEST_CANCEL:
            return HAKO_SERVICE_SERVER_API_REQUEST_CANCEL;
        default:
            break;
    }
    return HAKO_SERVICE_SERVER_API_EVENT_NONE;
}
int hako_asset_service_server_status(int service_id, int* status)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_status(): service_server_protocol is null" << std::endl;
        return -1;
    }
    if (status == nullptr) {
        std::cerr << "ERROR: hako_asset_service_server_status(): status is null" << std::endl;
        return -1;
    }
    *status = (int)service_server_protocol->state();
    return 0;
}

int hako_asset_service_server_get_request(int service_id, char** packet, size_t *packet_len)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_get_request(): service_server_protocol is null" << std::endl;
        return -1;
    }
    char* request_buffer = (char*)service_server_protocol->get_request();
    if (request_buffer == nullptr) {
        std::cerr << "ERROR: hako_asset_service_server_get_request(): request_buffer is null" << std::endl;
        return -1;
    }
    *packet = request_buffer;
    *packet_len = service_server_protocol->get_request_pdu_size();
    return 0;
}
int hako_asset_service_server_get_response_buffer(int service_id, char** packet, size_t *packet_len, int status, int result_code)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_get_response_buffer(): service_server_protocol is null" << std::endl;
        return -1;
    }
    char* response_buffer = (char*)service_server_protocol->get_response_buffer((hako::service::HakoServiceStatusType)status, (hako::service::HakoServiceResultCodeType)result_code);
    if (response_buffer == nullptr) {
        std::cerr << "ERROR: hako_asset_service_get_response_buffer(): response_buffer is null" << std::endl;
        return -1;
    }
    *packet = response_buffer;
    *packet_len = service_server_protocol->get_response_pdu_size();
    return 0;
}
int hako_asset_service_server_put_response(int service_id, char* packet, size_t packet_len)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_put_response(): service_server_protocol is null" << std::endl;
        return -1;
    }
    if (packet == nullptr || packet_len <= 0) {
        std::cerr << "ERROR: hako_asset_service_server_put_response(): packet is null or packet_len is invalid" << std::endl;
        return -1;
    }
    bool ret = service_server_protocol->reply(packet, packet_len);
    if (!ret) {
        std::cerr << "ERROR: hako_asset_service_server_put_response(): reply() failed" << std::endl;
        return -1;
    }
    return 0;
}
int hako_asset_service_server_is_canceled(int service_id)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_is_canceled(): service_server_protocol is null" << std::endl;
        return -1;
    }
    return service_server_protocol->state() == hako::service::HAKO_SERVICE_SERVER_STATE_CANCELING ? -1 : 0;
}
int hako_asset_service_server_set_progress(int service_id, int percentage)
{
    auto service_server_protocol = service_servers[service_id];
    if (!service_server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_set_status(): service_server_protocol is null" << std::endl;
        return -1;
    }
    service_server_protocol->put_progress(percentage);
    return 0;
}
