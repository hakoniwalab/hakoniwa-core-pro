#pragma once

#include "nlohmann/json.hpp"
#include <vector>
#include <string>

namespace hako::service::impl {


    struct HakoServiceImplType {
        bool is_initialized;
    };
    extern int initialize(const char* service_config_path);

    /*
     * Service server API
     */
    namespace server {
        extern int create(const char* serviceName);
        extern int get_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        extern int put_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
    }
    /*
     * Service client API
     */
    namespace client {
        extern int create(const char* serviceName, const char* clientName, int& client_id);
        extern int put_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        extern int get_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
    }
 
}
