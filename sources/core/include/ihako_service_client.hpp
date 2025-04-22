#pragma once

#include <string>
namespace hako::service {
    
    enum HakoServiceClientStateType {
        HAKO_SERVICE_CLIENT_STATE_IDLE = 0,
        HAKO_SERVICE_CLIENT_STATE_DOING,
        HAKO_SERVICE_CLIENT_STATE_CANCELING
    };

    enum HakoServiceClientEventType {
        HAKO_SERVICE_CLIENT_EVENT_NONE = 0,
        HAKO_SERVICE_CLIENT_RESPONSE_IN,
        HAKO_SERVICE_CLIENT_REQUEST_TIMEOUT,
        HAKO_SERVICE_CLIENT_REQUEST_CANCEL_DONE
    };

    class IHakoServiceClient {
        public:
            virtual ~IHakoServiceClient() = default;
            virtual bool initialize(const char* serviceName, const char* clientName, const char* assetName) = 0;

            virtual char* recv_response() = 0;
            virtual bool send_request(void* packet, int packet_len) = 0;
            virtual HakoServiceClientStateType get_state() = 0;

            virtual int get_service_id() = 0;
            virtual int get_asset_id() = 0;
            virtual int get_client_id() = 0;
            virtual void* get_request_buffer() = 0;
            virtual void* get_response_buffer() = 0;

            virtual bool event_start_service() = 0;
            virtual bool event_done_service() = 0;
            virtual bool event_cancel_service() = 0;

            virtual int get_request_pdu_size() = 0;
            virtual int get_response_pdu_size() = 0;

            virtual std::string get_service_name() = 0;
            virtual std::string get_client_name() = 0;
    };
}