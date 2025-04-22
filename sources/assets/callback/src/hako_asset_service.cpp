#include "hako_pro.hpp"
#include "hako_asset_service.h"
#include "hako_service_protocol.hpp"
#include "hako_service_impl.hpp"
#include <vector>
#include <unordered_map>

int hako_asset_service_initialize(const char* service_config_path)
{
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_service_initialize(): pro_data is null" << std::endl;
        return -1;
    }
    if (pro_data->initialize_service(service_config_path) == false) {
        std::cerr << "ERROR: hako_asset_service_initialize(): pro_data->initialize() failed" << std::endl;
        return -1;
    }
    return 0;
}
