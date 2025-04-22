#pragma once

#include "ihako_service_server.hpp"
#include "hako_service_protocol.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include "hako.hpp"
#include <memory>

namespace hako::service {

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
            void* get_response_buffer(HakoServiceStatusType status, HakoServiceResultCodeType result_code);
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
}