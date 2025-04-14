#pragma once

#include "nlohmann/json.hpp"
#include "hako_asset.hpp"
#include <vector>
#include <string>

namespace hako::service::impl {

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
    

    extern int initialize(const char* service_config_path, std::shared_ptr<hako::IHakoAssetController> hako_asset);
    extern bool get_services(std::vector<Service>& services);

    /*
     * Service server API
     */
    namespace server {
        struct HakoServiceImplType {
            bool is_initialized;
            nlohmann::json param;
            std::vector<Service> services;
        };
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
