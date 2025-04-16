#pragma once

#include "nlohmann/json.hpp"
#include "hako_pro_data.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include "hako_service.hpp"
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
    class HakoServiceServer: public IHakoServiceServer {
        public:
            HakoServiceServer() = default;
            ~HakoServiceServer() = default;
            /*
             * must be called after on_pdu_data_create(),
             * i.e, initialize() callback on HakoAsset.
             */
            void initialize(const char* serviceName, const char* assetName) override;

            char* recv_request(int clinet_id) override;
            bool send_response(int client_id) override;
            int get_current_client_id() override { return current_client_id_; }
            void next_client() override { current_client_id_ = (current_client_id_ + 1) % max_clients_; }
            HakoServiceServerStateType get_state() override { return state_[current_client_id_]; }

            bool cancel_service(int client_id) override { return event_cancel_service(client_id); }

            int get_service_id() override { return service_id_; }
            int get_asset_id() override { return asset_id_; }
            int get_request_pdu_size() override { return request_pdu_size_; }
            int get_response_pdu_size() override { return response_pdu_size_; }
            int get_max_clients() override { return max_clients_; }
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
            int current_client_id_ = 0;
            std::vector<HakoServiceServerStateType> state_;

            char* get_request_pdu_buffer() { return request_pdu_buffer_; }
            char* get_response_pdu_buffer() { return response_pdu_buffer_; }

            /*
             * EVENT APIs
             */
            bool event_start_service(int client_id);
            bool event_done_service(int client_id);
            bool event_cancel_service(int client_id);

            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceRequestHeader request_header_;
            HakoCpp_ServiceResponseHeader response_header_;
    };
    /*
     * Client Class
     */
    class HakoServiceClient: public IHakoServiceClient {
        public:
            HakoServiceClient() = default;
            ~HakoServiceClient() = default;
            void initialize(const char* serviceName, const char* clientName, const char* assetName) override;

            bool send_request();//TODO
            char* recv_response();//TODO
            bool cancel_service();//TODO timeout

            int get_service_id() { return service_id_; }
            int get_asset_id() { return asset_id_; }
            int get_request_pdu_size() { return request_pdu_size_; }
            int get_response_pdu_size() { return response_pdu_size_; }
            int get_client_id() { return client_id_; }
        private:
            int service_id_ = -1;
            int asset_id_ = -1;
            int client_id_ = -1;
            int request_pdu_size_ = 0;
            int response_pdu_size_ = 0;
            char* request_pdu_buffer_ = nullptr;
            char* response_pdu_buffer_ = nullptr;
            std::string service_name_;
            std::string client_name_;
            char* get_request_pdu_buffer() { return request_pdu_buffer_; }
            char* get_response_pdu_buffer() { return response_pdu_buffer_; }

            /*
             * EVENT APIs
             */
            HakoServiceClientStateType state_ = HAKO_SERVICE_CLIENT_STATE_IDLE;
            bool event_start_service();
            bool event_done_service();
            bool event_cancel_service();
            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceRequestHeader request_header_;
            HakoCpp_ServiceResponseHeader response_header_;
    };
 
}
