#pragma once

#include "nlohmann/json.hpp"
#include "hako_pro_data.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include "hako_service.hpp"
#include <vector>
#include <string>
#include <memory>

namespace hako::service::impl {


    struct HakoServiceImplType {
        bool is_initialized;
    };
    extern int initialize(const char* service_config_path);

    /*
     * Server Class
     */
    class HakoServiceServer: public IHakoServiceServer {
        public:
            HakoServiceServer() = default;
            ~HakoServiceServer() = default;
            /*
             * must be called after on_pdu_data_create(),
             * i.e, initialize() callback on HakoAsset.
             */
            bool initialize(const char* serviceName, const char* assetName) override;

            char* recv_request(int clinet_id) override;
            bool send_response(int client_id, void* packet, int packet_len)  override;
            int get_current_client_id() override { return current_client_id_; }
            void next_client() override { current_client_id_ = (current_client_id_ + 1) % max_clients_; }
            HakoServiceServerStateType get_state() override { return state_[current_client_id_]; }


            int get_service_id() override { return service_id_; }
            int get_asset_id() override { return asset_id_; }
            void* get_request_buffer() override { return get_request_pdu_buffer(); }
            void* get_response_buffer() override { return get_response_pdu_buffer(); }
            int get_request_pdu_size() override { return request_pdu_size_; }
            int get_response_pdu_size() override { return response_pdu_size_; }

            /*
             * EVENT APIs
             */
            bool event_start_service(int client_id) override;
            bool event_done_service(int client_id) override;
            bool event_cancel_service(int client_id) override;

            std::string get_service_name() override { return service_name_; }
            std::string get_client_name(int client_id) override;
            bool is_exist_client(std::string client_name) override;
            
        private:
            int service_id_ = -1;
            int asset_id_ = -1;
            int request_pdu_size_ = 0;
            int response_pdu_size_ = 0;
            std::unique_ptr<char[]> request_pdu_buffer_;
            std::unique_ptr<char[]> response_pdu_buffer_;
            int max_clients_ = 0;
            std::string service_name_;
            std::string asset_name_;
            int current_client_id_ = 0;
            std::vector<HakoServiceServerStateType> state_;

            char* get_request_pdu_buffer() { return request_pdu_buffer_.get(); }
            char* get_response_pdu_buffer() { return response_pdu_buffer_.get(); }
            int get_max_clients() { return max_clients_; }
    };
    /*
     * Client Class
     */
    class HakoServiceClient: public IHakoServiceClient {
        public:
            HakoServiceClient() = default;
            ~HakoServiceClient() = default;
            bool initialize(const char* serviceName, const char* clientName, const char* assetName) override;

            char* recv_response() override;
            bool send_request(void* packet, int packet_len) override;
            HakoServiceClientStateType get_state() override { return state_; }


            int get_service_id() override { return service_id_; }
            int get_asset_id() override { return asset_id_; }
            int get_client_id() override { return client_id_; }
            void* get_request_buffer() override { return request_pdu_buffer_.get(); }
            void* get_response_buffer() override { return response_pdu_buffer_.get(); }

            bool event_start_service() override;
            bool event_done_service() override;
            bool event_cancel_service() override;
            std::string get_service_name() override { return service_name_; }
            std::string get_client_name() override { return client_name_; }

            int get_request_pdu_size() override { return request_pdu_size_; }
            int get_response_pdu_size() override { return response_pdu_size_; }
        private:
            int service_id_ = -1;
            int asset_id_ = -1;
            int client_id_ = -1;
            int request_pdu_size_ = 0;
            int response_pdu_size_ = 0;
            std::unique_ptr<char[]> request_pdu_buffer_;
            std::unique_ptr<char[]> response_pdu_buffer_;
            std::string service_name_;
            std::string client_name_;

            /*
             * EVENT APIs
             */
            HakoServiceClientStateType state_ = HAKO_SERVICE_CLIENT_STATE_IDLE;

        };
 
}
