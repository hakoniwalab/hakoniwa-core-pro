#pragma once

#include "ihako_service_client.hpp"
#include "hako_service_protocol.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceRequestHeader.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_ServiceResponseHeader.hpp"
#include "pdu_convertor.hpp"
#include "hako_pro.hpp"
#include <memory>

namespace hako::service {
    
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
            bool  request(char* packet, int packet_len, int timeout_msec);
            bool  cancel_request();
            int   get_progress() { return percentage_; }
            int  get_service_id() { return client_->get_service_id(); }
            int get_client_id() { return client_->get_client_id(); }
            std::string get_service_name() { return client_->get_service_name(); }
            std::string get_client_name() { return client_->get_client_name(); }
            void* get_response_buffer() { return response_user_buffer_.get(); }
            int get_response_pdu_size() { return client_->get_response_pdu_size(); }
            int get_request_channel_id() { return client_->get_request_channel_id(); }
            int get_response_channel_id() { return client_->get_response_channel_id(); }

        private:
            std::shared_ptr<IHakoServiceClient> client_;
            bool recv_response(HakoCpp_ServiceResponseHeader& header, bool& data_recv_in);
            bool validate_header(HakoCpp_ServiceResponseHeader& header);
            bool copy_user_buffer(const HakoCpp_ServiceResponseHeader& header);
            bool set_request_header(HakoCpp_ServiceRequestHeader& header, HakoServiceOperationCodeType opcode, int poll_interval_msec);
            bool send_request(HakoServiceOperationCodeType opcode, int poll_interval_msec);
            HakoTimeType get_current_time_usec() {
                auto pro_data = hako::data::pro::hako_pro_get_data();
                if (!pro_data) {
                    std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
                    return 0;
                }
                return pro_data->get_world_time_usec();
            }
            bool is_heartbeat_timeout() {
                if (timeout_msec_ <= 0) {
                    return false;
                }
                HakoTimeType current_time = get_current_time_usec();
                if ((current_time - heartbeat_confirmed_time_) > (timeout_msec_ * 1000)) {
                    return true;
                }
                return false;
            }

            /*
             * packet
             */
            hako::pdu::PduConvertor<HakoCpp_ServiceRequestHeader, hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader> convertor_request_;
            hako::pdu::PduConvertor<HakoCpp_ServiceResponseHeader, hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader> convertor_response_;
            HakoCpp_ServiceResponseHeader response_header_;
            HakoCpp_ServiceRequestHeader request_header_;
            std::unique_ptr<char[]> request_user_buffer_;
            std::unique_ptr<char[]> response_user_buffer_;
            int percentage_ = 0;
            int request_id_ = 0;
            int timeout_msec_ = 0;
            int heartbeat_confirmed_time_ = 0;
            HakoTimeType last_poll_time_ = 0;
    };
}