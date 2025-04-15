#pragma once

#include "nlohmann/json.hpp"
#include "hako_pro_data.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include <vector>
#include <string>

namespace hako::service::impl {


    struct HakoServiceImplType {
        bool is_initialized;
    };
    extern int initialize(const char* service_config_path);
    enum HakoServiceServerStateType {
        HAKO_SERVICE_SERVER_STATE_IDLE = 0,
        HAKO_SERVICE_SERVER_STATE_DOING,
        HAKO_SERVICE_SERVER_STATE_CANCELING
    };
    enum HakoServiceClientStateType {
        HAKO_SERVICE_CLIENT_STATE_IDLE = 0,
        HAKO_SERVICE_CLIENT_STATE_DOING,
        HAKO_SERVICE_CLIENT_STATE_CANCELING
    };

    enum HakoServiceServerEventType {
        HAKO_SERVICE_SERVER_EVENT_NONE = 0,
        HAKO_SERVICE_SERVER_REQUEST_IN,
        HAKO_SERVICE_SERVER_REQUEST_CANCEL
    };
    enum HakoServiceClientEventType {
        HAKO_SERVICE_CLIENT_EVENT_NONE = 0,
        HAKO_SERVICE_CLIENT_RESPONSE_IN,
        HAKO_SERVICE_CLIENT_REQUEST_TIMEOUT,
        HAKO_SERVICE_CLIENT_REQUEST_CANCEL_DONE
    };
    class HakoServiceServer;
    class HakoServiceServerProtocol {
        public:
            HakoServiceServerProtocol(std::shared_ptr<HakoServiceServer> server)
            {
                server_ = server;
            }
            ~HakoServiceServerProtocol() = default;

            HakoServiceServerEventType run();
            HakoServiceServerStateType state();
            void* get_request();
            void  put_reply(void* packet, int result_code);
            void  cancel_done();
            void  put_progress(int percentage);

        private:
            std::shared_ptr<HakoServiceServer> server_;
    };
    class HakoServiceClient;
    class HakoServiceClientProtocol {
        public:
            HakoServiceClientProtocol(std::shared_ptr<HakoServiceClient> client)
            {
                client_ = client;
            }
            ~HakoServiceClientProtocol() = default;

            HakoServiceClientEventType run();
            HakoServiceClientStateType state();
            bool  put_request(void* packet);
            void* get_response();
            void  cancel_request();
            int   get_progress();

        private:
            std::shared_ptr<HakoServiceClient> client_;
    };

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

            char* recv_request(int clinet_id);
            bool send_response(int client_id);
            int get_current_client_id() { return current_client_id_; }
            void next_client() { current_client_id_ = (current_client_id_ + 1) % max_clients_; }
            HakoServiceServerStateType get_state() { return state_[current_client_id_]; }

            bool cancel_service(int client_id) { return event_cancel_service(client_id); }

            int get_service_id() { return service_id_; }
            int get_asset_id() { return asset_id_; }
            int get_request_pdu_size() { return request_pdu_size_; }
            int get_response_pdu_size() { return response_pdu_size_; }
            int get_max_clients() { return max_clients_; }
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
    class HakoServiceClient {
        public:
            HakoServiceClient() = default;
            ~HakoServiceClient() = default;
            void initialize(const char* serviceName, const char* clientName, const char* assetName);

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
