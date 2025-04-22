#include "hako_service_impl_server.hpp"
#include "hako_pro.hpp"
#include <fstream>
#include <iostream>

int hako::service::impl::initialize(const char* service_config_path)
{
    if (service_config_path == nullptr || *service_config_path == '\0') {
        std::cerr << "Error: service_config_path is not set." << std::endl;
        return -1;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return -1;
    }
    if (!pro_data->initialize_service(service_config_path)) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): initialize_service failed" << std::endl;
        return -1;
    }
    std::cout << "INFO: Service initialized successfully: " << service_config_path << std::endl;
    return 0;
}
