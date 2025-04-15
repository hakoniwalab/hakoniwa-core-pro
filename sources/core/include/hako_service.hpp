#pragma once

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
    };
    class IHakoServiceClient {
        public:
            virtual ~IHakoServiceClient() = default;
            virtual void initialize(const char* serviceName, const char* clientName, const char* assetName) = 0;
    };
}