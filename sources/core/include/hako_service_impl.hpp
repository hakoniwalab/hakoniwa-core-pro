#pragma once

#include "nlohmann/json.hpp"
#include "hako_pro_data.hpp"
#include <vector>
#include <string>

namespace hako::service::impl {


    struct HakoServiceImplType {
        bool is_initialized;
    };
    extern int initialize(const char* service_config_path);

    /*
     * Server Class
     */
    class HakoServiceServer {
        public:
            HakoServiceServer() = default;
            ~HakoServiceServer() = default;
            /*
             * must be called after on_pdu_data_create()
             */
            void initialize(const char* serviceName, const char* assetName);

        private:
            int service_id_ = -1;
            int asset_id_ = -1;
            int request_pdu_size_ = 0;
            int response_pdu_size_ = 0;
            char* request_pdu_buffer_ = nullptr;
            char* response_pdu_buffer_ = nullptr;
            int max_clients_ = 0;
            std::string service_name_;
            std::string asset_name_;
    };
    /*
     * Client Class
     */
    class HakoServiceClient {
        public:
            HakoServiceClient() = default;
            ~HakoServiceClient() = default;

        private:
    };


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
