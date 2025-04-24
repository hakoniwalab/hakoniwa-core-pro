#pragma once

#include "hako_asset_service.h"
#include "hako_service_protocol.hpp"
#include "pdu_convertor.hpp"
#include <string>

namespace hako::service {

/**
 * @brief Hakoniwa Service Client Front-End Template
 *
 * A generic front-end class for interacting with Hakoniwa services
 * as a client, encapsulating PDU conversion, request/response handling,
 * and timeout/cancellation control.
 *
 * This wrapper allows RPC-style service use via strongly typed request/response.
 *
 * @tparam CppReqPacketType   C++ wrapper struct for request packet
 * @tparam CppResPacketType   C++ wrapper struct for response packet
 * @tparam CppReqBodyType     Request payload struct
 * @tparam CppResBodyType     Response payload struct
 * @tparam ConvertorReq       Request PDU converter
 * @tparam ConvertorRes       Response PDU converter
 */
template<typename CppReqPacketType, typename CppResPacketType, 
         typename CppReqBodyType, typename CppResBodyType, 
         typename ConvertorReq, typename ConvertorRes>
class HakoAssetServiceClient {
public:
    /**
     * @brief Constructor
     * @param assetName Asset name in Hakoniwa
     * @param serviceName Target service name to connect to
     * @param clientName Unique client identifier
     */
    HakoAssetServiceClient(const char* assetName, const char* serviceName, const char* clientName)
        : asset_name_(assetName), service_name_(serviceName), client_name_(clientName), service_client_handle_({})
    {
        service_client_handle_.service_id = -1;
        service_client_handle_.client_id = -1;
    }

    virtual ~HakoAssetServiceClient() = default;

    /**
     * @brief Initializes the client (connects to backend)
     * @return true on success, false on failure
     */
    bool initialize() {
        int ret = hako_asset_service_client_create(asset_name_.c_str(), service_name_.c_str(), client_name_.c_str(), &service_client_handle_);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_create() returns %d.\n", ret);
            return false;
        }
        return true;
    }

    /**
     * @brief Sends a request to the service server
     *
     * @param req_body Request payload (user struct)
     * @param timeout_msec Timeout in milliseconds (-1 = infinite)
     * @param poll_interval_msec Progress polling interval (-1 = none)
     * @return true on success, false on failure
     */
    bool request(CppReqBodyType& req_body, int timeout_msec = -1, int poll_interval_msec = -1) {
        char* request_buffer = nullptr;
        size_t request_buffer_len = 0;
        int ret = hako_asset_service_client_get_request_buffer(&service_client_handle_, &request_buffer, &request_buffer_len,
                                                               HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, poll_interval_msec);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_get_request_buffer() returns %d.\n", ret);
            return false;
        }

        ret = req_convertor_.pdu2cpp(request_buffer, req_packet_);
        if (ret < 0) {
            printf("ERORR: request_convertor.pdu2cpp() returns %d.\n", ret);
            return false;
        }

        req_packet_.body = req_body;

        ret = req_convertor_.cpp2pdu(req_packet_, request_buffer, request_buffer_len);
        if (ret < 0) {
            printf("ERORR: req_convertor_.cpp2pdu() returns %d.\n", ret);
            return false;
        }

        ret = hako_asset_service_client_call_request(&service_client_handle_, request_buffer, request_buffer_len, timeout_msec);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_call_request() returns %d.\n", ret);
            return false;
        }

        return true;
    }

    /**
     * @brief Polls the client for service response or timeout
     * @return Event code: one of HAKO_SERVICE_CLIENT_API_EVENT_* macros
     */
    int poll() {
        int event = hako_asset_service_client_poll(&service_client_handle_);
        if (event < 0) {
            printf("ERORR: hako_asset_service_client_poll() returns %d.\n", event);
            return -1;
        }

        if ((event == HAKO_SERVICE_CLIENT_API_RESPONSE_IN) || (event == HAKO_SERVICE_CLIENT_API_REQUEST_CANCEL_DONE)) {
            char* response_buffer = nullptr;
            size_t response_buffer_len = 0;
            int ret = hako_asset_service_client_get_response(&service_client_handle_, &response_buffer, &response_buffer_len, -1);
            if (ret < 0) {
                printf("ERORR: hako_asset_service_client_get_response() returns %d.\n", ret);
                return -1;
            }

            ret = res_convertor_.pdu2cpp(response_buffer, res_packet_);
            if (ret < 0) {
                printf("ERORR: response_convertor.pdu2cpp() returns %d.\n", ret);
                return -1;
            }
        }
        return event;
    }

    /** Utility: checks if event = NONE */
    bool is_no_event(int event) { return event == HAKO_SERVICE_CLIENT_API_EVENT_NONE; }

    /** Utility: checks if event = timeout */
    bool is_request_timeout(int event) { return event == HAKO_SERVICE_CLIENT_API_REQUEST_TIMEOUT; }

    /** Utility: checks if cancel completed */
    bool is_request_cancel_done(int event) { return event == HAKO_SERVICE_CLIENT_API_REQUEST_CANCEL_DONE; }

    /** Utility: checks if valid response received */
    bool is_response_in(int event) { return event == HAKO_SERVICE_CLIENT_API_RESPONSE_IN; }

    /**
     * @brief Returns current client status
     * @return Status code or -1 on failure
     */
    int status() {
        int stat = 0;
        int ret = hako_asset_service_client_status(&service_client_handle_, &stat);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_status() returns %d.\n", ret);
            return -1;
        }
        return stat;
    }

    /**
     * @brief Retrieve the parsed response body
     * @return C++ response body object
     */
    CppResBodyType get_response() {
        return res_packet_.body;
    }

    /**
     * @brief Send a cancel request to the server
     * @return true on success, false on failure
     */
    bool cancel_request() {
        int ret = hako_asset_service_client_cancel_request(&service_client_handle_);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_cancel_request() returns %d.\n", ret);
            return false;
        }
        return true;
    }

private:
    hako::pdu::PduConvertor<CppReqPacketType, ConvertorReq> req_convertor_;
    hako::pdu::PduConvertor<CppResPacketType, ConvertorRes> res_convertor_;
    std::string asset_name_;
    std::string service_name_;
    std::string client_name_;
    CppReqPacketType req_packet_;
    CppResPacketType res_packet_;
    HakoServiceHandleType service_client_handle_;
};

} // namespace hako::service

/** Macro to resolve C++ packet PDU type */
#define HAKO_SERVICE_CLIENT_TYPE(type) hako::pdu::msgs::hako_srv_msgs::type

/** Client template instantiation helper macro */
#define HakoAssetServiceClientTemplateType(SRVNAME) \
    hako::service::HakoAssetServiceClient< \
        HakoCpp_##SRVNAME##RequestPacket, \
        HakoCpp_##SRVNAME##ResponsePacket, \
        HakoCpp_##SRVNAME##Request, \
        HakoCpp_##SRVNAME##Response, \
        HAKO_SERVICE_CLIENT_TYPE(SRVNAME##RequestPacket), \
        HAKO_SERVICE_CLIENT_TYPE(SRVNAME##ResponsePacket)>
