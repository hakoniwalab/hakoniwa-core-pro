#pragma once

#include "hako_asset_service.h"
#include "hako_service_protocol.hpp"
#include "pdu_convertor.hpp"
#include <string>

namespace hako::service
{
    template<typename CppReqPacketType, typename CppResPacketType, 
            typename CppReqBodyType, typename CppResBodyType, 
            typename ConvertorReq, typename ConvertorRes>
    class HakoAssetServiceClient
    {
        public:
        HakoAssetServiceClient(const char* assetName, const char* serviceName, const char* clientName)
            : asset_name_(assetName), service_name_(serviceName), client_name_(clientName), service_client_handle_({})
            {
                service_client_handle_.service_id = -1;
                service_client_handle_.client_id = -1;
            }
            virtual ~HakoAssetServiceClient()
            {
            }
            bool initialize()
            {
                int ret = hako_asset_service_client_create(asset_name_.c_str(), service_name_.c_str(), client_name_.c_str(), &service_client_handle_);
                if (ret < 0) {
                    printf("ERORR: hako_asset_service_client_create() returns %d.\n", ret);
                    return false;
                }
                return true;
            }
            bool request(CppReqBodyType& req_body, int timeout_msec = -1, int poll_interval_msec = -1)
            {
                char* request_buffer = nullptr;
                size_t request_buffer_len = 0;
                int ret = hako_asset_service_client_get_request_buffer(&service_client_handle_, &request_buffer, &request_buffer_len, 
                    HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, poll_interval_msec);
                if (ret < 0) {
                    printf("ERORR: hako_asset_service_client_get_request_buffer() returns %d.\n", ret);
                    return false;
                }
                //std::cout << "INFO: hako_asset_service_client_get_request_buffer() buffer_len= " << request_buffer_len << std::endl;
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
            int poll()
            {
                int ret = hako_asset_service_client_poll(&service_client_handle_);
                if (ret < 0) {
                    printf("ERORR: hako_asset_service_client_poll() returns %d.\n", ret);
                    return -1;
                }
                if (ret == HAKO_SERVICE_CLIENT_API_RESPONSE_IN) {
                    char* response_buffer = nullptr;
                    size_t response_buffer_len = 0;
                    ret = hako_asset_service_client_get_response(&service_client_handle_, &response_buffer, &response_buffer_len, -1);
                    if (ret < 0) {
                        printf("ERORR: hako_asset_service_client_get_response() returns %d.\n", ret);
                        return -1;
                    }
                    ret = res_convertor_.pdu2cpp(response_buffer, res_packet_);
                    if (ret < 0) {
                        printf("ERORR: response_convertor.pdu2cpp() returns %d.\n", ret);
                        return -1;
                    }
                    if (res_packet_.header.result_code == HAKO_SERVICE_RESULT_CODE_CANCELED) {
                        printf("ERORR: request is canceled.\n");
                        return HAKO_SERVICE_CLIENT_API_REQUEST_CANCEL_DONE;
                    }
                    return HAKO_SERVICE_CLIENT_API_RESPONSE_IN;
                }
                return ret;
            }
            int status()
            {
                int status = 0;
                int ret = hako_asset_service_client_status(&service_client_handle_, &status);
                if (ret < 0) {
                    printf("ERORR: hako_asset_service_client_status() returns %d.\n", ret);
                    return -1;
                }
                return status;
            }
            CppResBodyType get_response()
            {
                return res_packet_.body;
            }
            bool cancel_request()
            {
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
}
#define HAKO_SERVICE_CLIENT_TYPE(type) hako::pdu::msgs::hako_srv_msgs::type
#define HakoAssetServiceClientTemplateType(SRVNAME) \
    hako::service::HakoAssetServiceClient< \
        HakoCpp_##SRVNAME##RequestPacket, \
        HakoCpp_##SRVNAME##ResponsePacket, \
        HakoCpp_##SRVNAME##Request, \
        HakoCpp_##SRVNAME##Response, \
        HAKO_SERVICE_CLIENT_TYPE(SRVNAME##RequestPacket), \
        HAKO_SERVICE_CLIENT_TYPE(SRVNAME##ResponsePacket)> 
