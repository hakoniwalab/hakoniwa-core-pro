#pragma once

#include "nlohmann/json.hpp"
#include "hako_pro_data.hpp"
#include "pdu_convertor.hpp"
#include "ihako_service_client.hpp"
#include <vector>
#include <string>
#include <memory>

namespace hako::service::impl {


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
            void* get_temp_request_buffer() override { return request_pdu_buffer_.get(); }
            void* get_temp_response_buffer() override { return response_pdu_buffer_.get(); }

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
