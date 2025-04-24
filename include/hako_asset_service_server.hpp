#pragma once

#include "hako_asset_service.h"
#include "pdu_convertor.hpp"
#include <string>

namespace hako::service
{
    template<typename CppReqPacketType, typename CppResPacketType, 
            typename CppReqBodyType, typename CppResBodyType, 
            typename ConvertorReq, typename ConvertorRes>
    class HakoAssetServiceServer
    {
        public:
            HakoAssetServiceServer(const char* assetName, const char* serviceName) 
                : asset_name_(assetName), service_name_(serviceName)
            {
            }
            virtual ~HakoAssetServiceServer()
            {
            }
            bool initialize()
            {
                service_id_ = hako_asset_service_server_create(asset_name_.c_str(), service_name_.c_str());
                if (service_id_ < 0) {
                    std::cerr << "ERROR: hako_asset_service_server_create() returns " << service_id_ << std::endl;
                    return false;
                }
                return true;
            }
            int poll()
            {
                int ret = hako_asset_service_server_poll(service_id_);
                if (ret < 0) {
                    std::cerr << "ERROR: hako_asset_service_server_poll() returns " << ret << std::endl;
                    return false;
                }
                //std::cout << "INFO: hako_asset_service_server_poll(): " << ret << std::endl;
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
            bool is_no_event(int event)
            {
                if (event == HAKO_SERVICE_SERVER_API_EVENT_NONE) {
                    return true;
                }
                return false;
            }
            bool is_request_in(int event)
            {
                if (event == HAKO_SERVICE_SERVER_API_REQUEST_IN) {
                    return true;
                }
                return false;
            }
            bool is_request_cancel(int event)
            {
                if (event == HAKO_SERVICE_SERVER_API_REQUEST_CANCEL) {
                    return true;
                }
                return false;
            }
            CppReqBodyType get_request()
            {
                return req_packet_.body;
            }
            bool normal_reply(CppResBodyType& res_body)
            {
                return reply(res_body, HAKO_SERVICE_API_RESULT_CODE_OK);
            }
            bool cancel_reply(CppResBodyType& res_body)
            {
                return reply(res_body, HAKO_SERVICE_API_RESULT_CODE_CANCELED);
            }
        private:
            bool reply(CppResBodyType& res_body, int result_code = HAKO_SERVICE_API_RESULT_CODE_OK)
            {
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
                    std::cerr << "ERROR: response_convertor.pdu2cpp() returns " << ret << std::endl;
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
}

#define HAKO_SERVICE_SERVER_TYPE(type) hako::pdu::msgs::hako_srv_msgs::type
#define HakoAssetServiceServerTemplateType(SRVNAME) \
    hako::service::HakoAssetServiceServer< \
        HakoCpp_##SRVNAME##RequestPacket, \
        HakoCpp_##SRVNAME##ResponsePacket, \
        HakoCpp_##SRVNAME##Request, \
        HakoCpp_##SRVNAME##Response, \
        HAKO_SERVICE_SERVER_TYPE(SRVNAME##RequestPacket), \
        HAKO_SERVICE_SERVER_TYPE(SRVNAME##ResponsePacket)> 
