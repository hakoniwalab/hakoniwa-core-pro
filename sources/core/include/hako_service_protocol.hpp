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
    class HakoServiceServerProtocol {
        public:
            HakoServiceServerProtocol(std::shared_ptr<IHakoServiceServer> server)
            {
                server_ = server;
            }
            ~HakoServiceServerProtocol() {
                if (request_pdu_buffer_) {
                    delete[] request_pdu_buffer_;
                }
                if (response_pdu_buffer_) {
                    delete[] response_pdu_buffer_;
                }
            };
            /*
             * must be called after on_pdu_data_create(),
             * i.e, initialize() callback on HakoAsset.
             */
            bool initialize(const char* serviceName, const char* assetName);
            HakoServiceServerEventType poll();
            HakoServiceServerStateType state() { return server_->get_state(); }
            void* get_request();
            bool  put_reply(void* packet, int packet_len, int result_code);
            void  cancel_done();
            void  put_progress(int percentage) { percentage_ = percentage; }

        private:
            std::shared_ptr<IHakoServiceServer> server_;
            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceRequestHeader request_header_;
            HakoCpp_ServiceResponseHeader response_header_;
            HakoServiceServerEventType handle_polling();
            bool recv_request(int client_id, HakoCpp_ServiceRequestHeader& header, bool& data_recv_in);
            bool validate_header(HakoCpp_ServiceRequestHeader& header);
            bool copy_user_buffer(const HakoCpp_ServiceRequestHeader& header);
            char* request_pdu_buffer_ = nullptr;
            char* response_pdu_buffer_ = nullptr;
            int percentage_ = 0;
            HakoTimeType last_poll_time_ = 0;

    };
    class HakoServiceClientProtocol {
        public:
            HakoServiceClientProtocol(std::shared_ptr<IHakoServiceClient> client)
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
            std::shared_ptr<IHakoServiceClient> client_;
            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceRequestHeader request_header_;
            HakoCpp_ServiceResponseHeader response_header_;
    };
}