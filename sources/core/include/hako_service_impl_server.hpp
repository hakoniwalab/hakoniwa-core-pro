#pragma once

#include "nlohmann/json.hpp"
#include "hako_pro_data.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include "ihako_service_server.hpp"
#include <vector>
#include <string>
#include <memory>

namespace hako::service::impl {
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
}
