#include "hako_service_impl.hpp"
#include <fstream>
#include <iostream>

hako::service::impl::HakoServiceImplType hako_service_instance;

int hako::service::impl::initialize(const char* service_config_path)
{
    hako_service_instance.is_initialized = false;
    std::ifstream ifs(service_config_path);
    
    if (!ifs.is_open()) {
        std::cerr << "Error: Failed to open service config file: " << service_config_path << std::endl;
        return -1;
    }

    try {
        ifs >> hako_service_instance.param;
        for (const auto& item : hako_service_instance.param["services"]) {
            Service s;
            s.name = item["name"];
            s.type = item["type"];
            s.maxClients = item["maxClients"];
            hako_service_instance.services.push_back(s);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse service config JSON: " << e.what() << std::endl;
        return -1;
    }

    hako_service_instance.is_initialized = true;
    return 0;
}

bool hako::service::impl::get_services(std::vector<hako::service::impl::Service>& services)
{
    if (!hako_service_instance.is_initialized) {
        return false;
    }
    services = hako_service_instance.services;
    return true;
}
