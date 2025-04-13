#pragma once

#include "hako_service.h"
#include "nlohmann/json.hpp"
#include "hako_asset.hpp"
#include <vector>
#include <string>

namespace hako::service::impl {

    #define HAKO_SERVICE_SERVER_CHANNEL_ID 0
    #define HAKO_SERVICE_CLIENT_CHANNEL_ID 1
    struct Service {
        std::string name;
        std::string type;
        int maxClients;
    
        int pdu_size_server_base;
        int pdu_size_client_base;
        int pdu_size_server_heap;
        int pdu_size_client_heap;
    
        size_t server_total_size;
        size_t client_total_size;
    };
    

    extern bool get_services(std::vector<Service>& services);

    struct HakoServiceImplType {
        bool is_initialized;
        nlohmann::json param;
        std::vector<Service> services;
    };

    extern int initialize(const char* service_config_path, std::shared_ptr<hako::IHakoAssetController> hako_asset);
}
