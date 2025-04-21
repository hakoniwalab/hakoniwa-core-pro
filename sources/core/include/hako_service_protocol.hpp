#pragma once

#include "hako_service.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include "hako.hpp"
#include <memory>

namespace hako::service {
    enum HakoServiceOperationCodeType {
        HAKO_SERVICE_OPERATION_CODE_REQUEST = 0,
        HAKO_SERVICE_OPERATION_CODE_CANCEL,
        HAKO_SERVICE_OPERATION_NUM
    };
    enum HakoServiceStatusType {
        HAKO_SERVICE_STATUS_NONE = 0,
        HAKO_SERVICE_STATUS_DOING,
        HAKO_SERVICE_STATUS_CANCELING,
        HAKO_SERVICE_STATUS_DONE,
        HAKO_SERVICE_STATUS_ERROR,
        HAKO_SERVICE_STATUS_NUM
    };
    enum HakoServiceResultCodeType {
        HAKO_SERVICE_RESULT_CODE_OK = 0,
        HAKO_SERVICE_RESULT_CODE_ERROR,
        HAKO_SERVICE_RESULT_CODE_TIMEOUT,
        HAKO_SERVICE_RESULT_CODE_CANCEL,
        HAKO_SERVICE_RESULT_CODE_INVALID,
        HAKO_SERVICE_RESULT_CODE_BUSY,
        HAKO_SERVICE_RESULT_CODE_NOT_FOUND,
        HAKO_SERVICE_RESULT_CODE_NOT_SUPPORTED,
        HAKO_SERVICE_RESULT_CODE_NOT_READY,
        HAKO_SERVICE_RESULT_CODE_NOT_AVAILABLE,
        HAKO_SERVICE_RESULT_CODE_NOT_IMPLEMENTED,
        HAKO_SERVICE_RESULT_CODE_NUM
    };
    class HakoServiceServerProtocol {
        public:
            HakoServiceServerProtocol(std::shared_ptr<IHakoServiceServer> server)
            {
                server_ = server;
            }
            ~HakoServiceServerProtocol() {
            };
            /*
             * must be called after on_pdu_data_create(),
             * i.e, initialize() callback on HakoAsset.
             */
            bool initialize(const char* serviceName, const char* assetName);
            HakoServiceServerEventType poll();
            HakoServiceServerStateType state() { return server_->get_state(); }
            void* get_request();
            int get_request_pdu_size() { return server_->get_request_pdu_size(); }
            void* get_response_buffer() { return response_pdu_buffer_.get(); }
            int   get_response_pdu_size() { return server_->get_response_pdu_size(); }
            bool  set_response_header(HakoCpp_ServiceResponseHeader& header, HakoServiceStatusType status, HakoServiceResultCodeType result_code);
            bool  reply(char* packet, int packet_len);
            void  cancel_done();
            void  put_progress(int percentage) { percentage_ = percentage; }
            int   get_service_id() { return server_->get_service_id(); }
            std::string get_service_name() { return server_->get_service_name(); }

        private:
            std::shared_ptr<IHakoServiceServer> server_;
            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceRequestHeader request_header_;
            bool recv_request(int client_id, HakoCpp_ServiceRequestHeader& header, bool& data_recv_in);
            bool validate_header(HakoCpp_ServiceRequestHeader& header);
            bool copy_user_buffer(const HakoCpp_ServiceRequestHeader& header);
            bool send_response(HakoServiceStatusType status, HakoServiceResultCodeType result_code);
            std::unique_ptr<char[]> request_pdu_buffer_;
            std::unique_ptr<char[]> response_pdu_buffer_;
            int percentage_ = 0;
            HakoTimeType last_poll_time_ = 0;

    };
    class HakoServiceClientProtocol {
        public:
            HakoServiceClientProtocol(std::shared_ptr<IHakoServiceClient> client)
            {
                client_ = client;
            }
            ~HakoServiceClientProtocol() {
            };
            /*
             * must be called after on_pdu_data_create(),
             * i.e, initialize() callback on HakoAsset.
             */
            bool initialize(const char* serviceName, const char* clientName, const char* assetName);

            HakoServiceClientEventType poll();
            HakoServiceClientStateType state() { return client_->get_state(); }
            void* get_response();
            void* get_request_buffer(int opcode, int poll_interval_msec);
            int   get_request_pdu_size() { return client_->get_request_pdu_size(); }
            bool  set_request_header(HakoCpp_ServiceRequestHeader& header, HakoServiceOperationCodeType opcode, int poll_interval_msec);
            bool  request(char* packet, int packet_len);
            void  cancel_request();
            int   get_progress() { return percentage_; }
            int  get_service_id() { return client_->get_service_id(); }
            int get_client_id() { return client_->get_client_id(); }
            std::string get_service_name() { return client_->get_service_name(); }
            std::string get_client_name() { return client_->get_client_name(); }
            void* get_response_buffer() { return response_pdu_buffer_.get(); }
            int get_response_pdu_size() { return client_->get_response_pdu_size(); }

        private:
            std::shared_ptr<IHakoServiceClient> client_;
            bool recv_response(HakoCpp_ServiceResponseHeader& header, bool& data_recv_in);
            bool validate_header(HakoCpp_ServiceResponseHeader& header);
            bool copy_user_buffer(const HakoCpp_ServiceResponseHeader& header);
            bool send_request(HakoServiceOperationCodeType opcode, int poll_interval_msec);

            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceResponseHeader response_header_;
            HakoCpp_ServiceRequestHeader request_header_;
            std::unique_ptr<char[]> request_pdu_buffer_;
            std::unique_ptr<char[]> response_pdu_buffer_;
            int percentage_ = 0;
            int request_id_ = 0;
            HakoTimeType last_poll_time_ = 0;
    };
}