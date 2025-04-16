#pragma once

#include "hako_service.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include <memory>

namespace hako::service {
    class HakoServiceServerProtocol {
        public:
            HakoServiceServerProtocol(std::shared_ptr<IHakoServiceServer> server)
            {
                server_ = server;
            }
            ~HakoServiceServerProtocol() = default;
            /*
             * must be called after on_pdu_data_create(),
             * i.e, initialize() callback on HakoAsset.
             */
            bool initialize(const char* serviceName, const char* assetName);
            HakoServiceServerEventType run();
            HakoServiceServerStateType state() { return server_->get_state(); }
            void* get_request();
            bool  put_reply(void* packet, int packet_len, int result_code);
            void  cancel_done();
            void  put_progress(int percentage);

        private:
            std::shared_ptr<IHakoServiceServer> server_;
            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceRequestHeader request_header_;
            HakoCpp_ServiceResponseHeader response_header_;
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