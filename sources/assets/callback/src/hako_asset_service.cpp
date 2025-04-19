#include "hako_impl.hpp"
#include "hako_asset_service.h"
#include "hako_service_protocol.hpp"
#include "hako_service_impl.hpp"
#include <vector>

std::vector<std::shared_ptr<hako::service::HakoServiceServerProtocol>> service_servers = {};
std::vector<std::shared_ptr<hako::service::HakoServiceServerProtocol>> service_clients = {};

int hako_asset_service_initialize(const char* service_config_path)
{
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_service_initialize(): pro_data is null" << std::endl;
        return -1;
    }
    if (pro_data->initialize_service(service_config_path) != 0) {
        std::cerr << "ERROR: hako_asset_service_initialize(): pro_data->initialize() failed" << std::endl;
        return -1;
    }
    return 0;
}

int hako_asset_service_server_create(const char* assetName, const char* serviceName)
{
    for (const auto& server : service_servers) {
        if (server->get_service_name() == serviceName) {
            std::cerr << "ERROR: hako_asset_service_server_create(): service already exists" << std::endl;
            return -1;
        }
    }
    auto server_impl = std::make_shared<hako::service::impl::HakoServiceServer>();
    if (!server_impl) {
        std::cerr << "ERROR: hako_asset_service_server_create(): server_impl is null" << std::endl;
        return -1;
    }
    auto server_protocol = std::make_shared<hako::service::HakoServiceServerProtocol>(server_impl);
    if (!server_protocol) {
        std::cerr << "ERROR: hako_asset_service_server_create(): server_protocol is null" << std::endl;
        return -1;
    }
    bool ret = server_protocol->initialize(serviceName, assetName);
    if (!ret) {
        std::cerr << "ERROR: hako_asset_service_server_create(): server_protocol->initialize() failed" << std::endl;
        return -1;
    }
    service_servers.push_back(server_protocol);
    return server_protocol->get_service_id();
}