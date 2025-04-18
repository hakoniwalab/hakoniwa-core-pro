#pragma once

#include <string>
namespace hako::service {
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

    class IHakoServiceServer {
        public:
            virtual ~IHakoServiceServer() = default;
            virtual void initialize(const char* serviceName, const char* assetName) = 0;
            virtual char* recv_request(int clinet_id) = 0;
            virtual bool send_response(int client_id, void* packet, int packet_len) = 0;
            virtual int get_current_client_id() = 0;
            virtual void next_client() = 0;
            virtual HakoServiceServerStateType get_state() = 0;

            virtual bool cancel_service(int client_id) = 0;

            virtual int get_service_id() = 0;
            virtual int get_asset_id() = 0;
            virtual void* get_request_buffer() = 0;
            virtual void* get_response_buffer() = 0;
            virtual bool event_start_service(int client_id);
            virtual bool event_done_service(int client_id);
            virtual bool event_cancel_service(int client_id);

            virtual int get_request_pdu_size() = 0;
            virtual int get_response_pdu_size() = 0;

            virtual std::string get_service_name() = 0;
            virtual std::string get_client_name(int client_id) = 0;
            virtual bool is_exist_client(std::string client_name);
    };
    class IHakoServiceClient {
        public:
            virtual ~IHakoServiceClient() = default;
            virtual void initialize(const char* serviceName, const char* clientName, const char* assetName) = 0;
    };
}