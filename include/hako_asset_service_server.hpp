#pragma once

#include "hako_asset_service.h"
#include "pdu_convertor.hpp"
#include <string>

namespace hako::service {

/**
 * @brief Hakoniwa Service Server Front-End Template
 *
 * This template class provides a simplified front-end interface
 * for implementing service servers in Hakoniwa.
 *
 * It wraps the low-level C API and handles PDU conversion,
 * request polling, and response publishing using C++ types.
 *
 * @tparam CppReqPacketType   Complete C++ request packet type
 * @tparam CppResPacketType   Complete C++ response packet type
 * @tparam CppReqBodyType     Request body structure
 * @tparam CppResBodyType     Response body structure
 * @tparam ConvertorReq       Converter class for request
 * @tparam ConvertorRes       Converter class for response
 */
template<typename CppReqPacketType, typename CppResPacketType, 
         typename CppReqBodyType, typename CppResBodyType, 
         typename ConvertorReq, typename ConvertorRes>
class HakoAssetServiceServer {
public:
    /**
     * @brief Constructor
     * @param assetName The asset name registered in Hakoniwa
     * @param serviceName The name of the service to provide
     */
    HakoAssetServiceServer(const char* assetName, const char* serviceName)
        : asset_name_(assetName), service_name_(serviceName) {}

    virtual ~HakoAssetServiceServer() = default;

    /**
     * @brief Initialize the server (register with Hakoniwa backend)
     * @return true on success, false on failure
     */
    bool initialize() {
        service_id_ = hako_asset_service_server_create(asset_name_.c_str(), service_name_.c_str());
        if (service_id_ < 0) {
            std::cerr << "ERROR: hako_asset_service_server_create() returns " << service_id_ << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Poll for incoming service requests
     * 
     * Automatically converts received PDU into internal request packet.
     * 
     * @return One of the HAKO_SERVICE_SERVER_API_* event codes
     */
    int poll() {
        int ret = hako_asset_service_server_poll(service_id_);
        if (ret < 0) {
            std::cerr << "ERROR: hako_asset_service_server_poll() returns " << ret << std::endl;
            return false;
        }
        if (ret == HAKO_SERVICE_SERVER_API_REQUEST_IN) {
            char* request_buffer = nullptr;
            size_t request_buffer_len = 0;
            ret = hako_asset_service_server_get_request(service_id_, &request_buffer, &request_buffer_len);
            if (ret < 0) {
                std::cerr << "ERROR: hako_asset_service_server_get_request() returns " << ret << std::endl;
                return false;
            }
            ret = req_convertor_.pdu2cpp(request_buffer, req_packet_);
            if (ret < 0) {
                std::cerr << "ERROR: req_convertor_.pdu2cpp() returns " << ret << std::endl;
                return false;
            }
            return HAKO_SERVICE_SERVER_API_REQUEST_IN;
        }
        return ret;
    }

    /** Utility check for: no event occurred */
    bool is_no_event(int event) {
        return event == HAKO_SERVICE_SERVER_API_EVENT_NONE;
    }

    /** Utility check for: new request received */
    bool is_request_in(int event) {
        return event == HAKO_SERVICE_SERVER_API_REQUEST_IN;
    }

    /** Utility check for: cancel request received */
    bool is_request_cancel(int event) {
        return event == HAKO_SERVICE_SERVER_API_REQUEST_CANCEL;
    }

    /**
     * @brief Retrieve the parsed request body
     * @return The body of the request
     */
    CppReqBodyType get_request() {
        return req_packet_.body;
    }

    /**
     * @brief Send a successful response
     * @param res_body The response content
     * @return true on success, false on failure
     */
    bool normal_reply(CppResBodyType& res_body) {
        return reply(res_body, HAKO_SERVICE_API_RESULT_CODE_OK);
    }

    /**
     * @brief Send a cancel-completed response
     * @param res_body The response content
     * @return true on success, false on failure
     */
    bool cancel_reply(CppResBodyType& res_body) {
        return reply(res_body, HAKO_SERVICE_API_RESULT_CODE_CANCELED);
    }

private:
    /**
     * @brief Internal function to send a reply with given result code
     */
    bool reply(CppResBodyType& res_body, int result_code) {
        char* response_buffer = nullptr;
        size_t response_buffer_len = 0;

        int ret = hako_asset_service_server_get_response_buffer(service_id_, &response_buffer, &response_buffer_len,
                                                                HAKO_SERVICE_API_STATUS_DONE, result_code);
        if (ret < 0) {
            std::cerr << "ERROR: hako_asset_service_server_get_response() returns " << ret << std::endl;
            return false;
        }

        ret = res_convertor_.pdu2cpp(response_buffer, res_packet_);
        if (ret < 0) {
            std::cerr << "ERROR: res_convertor_.pdu2cpp() returns " << ret << std::endl;
            return false;
        }

        res_packet_.body = res_body;

        ret = res_convertor_.cpp2pdu(res_packet_, response_buffer, response_buffer_len);
        if (ret < 0) {
            std::cerr << "ERROR: res_convertor_.cpp2pdu() returns " << ret << std::endl;
            return false;
        }

        ret = hako_asset_service_server_put_response(service_id_, response_buffer, response_buffer_len);
        if (ret < 0) {
            std::cerr << "ERROR: hako_asset_service_server_put_response() returns " << ret << std::endl;
            return false;
        }

        return true;
    }

    hako::pdu::PduConvertor<CppReqPacketType, ConvertorReq> req_convertor_;
    hako::pdu::PduConvertor<CppResPacketType, ConvertorRes> res_convertor_;
    int service_id_ = -1;
    std::string asset_name_;
    std::string service_name_;
    CppReqPacketType req_packet_;
    CppResPacketType res_packet_;
};

} // namespace hako::service

/** Shorthand macro to resolve PDU type */
#define HAKO_SERVICE_SERVER_TYPE(type) hako::pdu::msgs::hako_srv_msgs::type

/** Template instantiation helper */
#define HakoAssetServiceServerTemplateType(SRVNAME) \
    hako::service::HakoAssetServiceServer< \
        HakoCpp_##SRVNAME##RequestPacket, \
        HakoCpp_##SRVNAME##ResponsePacket, \
        HakoCpp_##SRVNAME##Request, \
        HakoCpp_##SRVNAME##Response, \
        HAKO_SERVICE_SERVER_TYPE(SRVNAME##RequestPacket), \
        HAKO_SERVICE_SERVER_TYPE(SRVNAME##ResponsePacket)>
