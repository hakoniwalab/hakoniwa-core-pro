#pragma once

#include "hako_service.h"
#include "nlohmann/json.hpp"
#include <vector>
#include <string>

namespace hako::service::impl {

    struct Service {
        std::string name;
        std::string type;
        int maxClients;
    };

    extern bool get_services(std::vector<Service>& services);

    struct HakoServiceImplType {
        bool is_initialized;
        nlohmann::json param;
        std::vector<Service> services;
    };

    extern int initialize(const char* service_config_path);
}
