#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_conductor.h"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"
#include "pdu_convertor.hpp"
#include "pdu_info.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
static inline void usleep(long microseconds) {
    Sleep(microseconds / 1000);
}
#else
#include <unistd.h>
#endif
static const char* asset_name = "Client";
static const char* service_config_path = "./examples/service/service.json";
static const char* service_name = "Service/Add";
static const char* service_client_name = "Client01";
static HakoServiceHandleType service_client_handle = {};
hako_time_t delta_time_usec = 1000 * 1000;

static int my_on_initialize(hako_asset_context_t* context)
{
    (void)context;
    printf("INFO: hako_asset_service_client_create()...\n");
    int ret = hako_asset_service_client_create(asset_name, service_name, service_client_name, &service_client_handle);
    if (ret < 0) {
        printf("ERORR: hako_asset_service_client_create() returns %d.\n", ret);
        return 1;
    }
    printf("INFO: hako_asset_service_client_create() service_id=%d client_id=%d.\n", service_client_handle.service_id, service_client_handle.client_id);
    return 0;
}
static int my_on_reset(hako_asset_context_t* context)
{
    (void)context;
    return 0;
}

static void debug_packet(HakoCpp_AddTwoIntsRequestPacket& request_packet)
{
    std::cout << "INFO: request_packet.header.request_id=" << request_packet.header.request_id << std::endl;
    std::cout << "INFO: request_packet.header.client_name=" << request_packet.header.client_name << std::endl;
    std::cout << "INFO: request_packet.header.service_name=" << request_packet.header.service_name << std::endl;
    std::cout << "INFO: request_packet.header.opcode=" << (int)request_packet.header.opcode << std::endl;
    std::cout << "INFO: request_packet.header.status_poll_interval_msec=" << request_packet.header.status_poll_interval_msec << std::endl;
    std::cout << "INFO: request_packet.body.a=" << request_packet.body.a << std::endl;
    std::cout << "INFO: request_packet.body.b=" << request_packet.body.b << std::endl;
}
static void debug_response_packet(HakoCpp_AddTwoIntsResponsePacket& response_packet)
{
    std::cout << "INFO: response_packet.header.request_id=" << response_packet.header.request_id << std::endl;
    std::cout << "INFO: response_packet.header.client_name=" << response_packet.header.client_name << std::endl;
    std::cout << "INFO: response_packet.header.service_name=" << response_packet.header.service_name << std::endl;
    std::cout << "INFO: response_packet.header.status=" << (int)response_packet.header.status << std::endl;
    std::cout << "INFO: response_packet.header.processing_percentage=" << (int)response_packet.header.processing_percentage << std::endl;
    std::cout << "INFO: response_packet.body.sum=" << response_packet.body.sum << std::endl;
}

static int my_on_manual_timing_control(hako_asset_context_t* context)
{
    (void)context;
    HakoCpp_AddTwoIntsRequestPacket request_packet;
    hako::pdu::PduConvertor<HakoCpp_AddTwoIntsRequestPacket, hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket> request_convertor;
    HakoCpp_AddTwoIntsResponsePacket response_packet;
    hako::pdu::PduConvertor<HakoCpp_AddTwoIntsResponsePacket, hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket> response_convertor;
    char* request_buffer = nullptr;
    size_t request_buffer_len = 0;
    int ret = hako_asset_service_client_get_request_buffer(&service_client_handle, &request_buffer, &request_buffer_len, HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, -1);
    if (ret < 0) {
        printf("ERORR: hako_asset_service_client_get_request_buffer() returns %d.\n", ret);
        return 1;
    }
    std::cout << "INFO: hako_asset_service_client_get_request_buffer() buffer_len= " << request_buffer_len << std::endl;
    ret = request_convertor.pdu2cpp(request_buffer, request_packet);
    if (ret < 0) {
        printf("ERORR: request_convertor.pdu2cpp() returns %d.\n", ret);
        return 1;
    }
    request_packet.body.a = 1;
    request_packet.body.b = 2;
    debug_packet(request_packet);
    while (true) {
        ret = hako_asset_service_client_poll(&service_client_handle);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_poll() returns %d.\n", ret);
            return 1;
        }
        std::cout << "INFO: hako_asset_service_client_poll() returns " << ret << std::endl;
        int status = 0;
        hako_asset_service_client_status(&service_client_handle, &status);
        if ((ret == HAKO_SERVICE_CLIENT_API_EVENT_NONE) && (status == HAKO_SERVICE_CLIENT_API_STATE_IDLE)) {
            ret = request_convertor.cpp2pdu(request_packet, request_buffer, request_buffer_len);
            if (ret < 0) {
                printf("ERORR: request_convertor.cpp2pdu() returns %d.\n", ret);
                return 1;
            }
            ret = hako_asset_service_client_call_request(&service_client_handle, request_buffer, request_buffer_len, -1);
            if (ret < 0) {
                printf("ERORR: hako_asset_service_client_call_request() returns %d.\n", ret);
                return 1;
            }
        }
        else if (ret == HAKO_SERVICE_CLIENT_API_RESPONSE_IN) {
            std::cout << "INFO: hako_asset_service_client_get_response()..." << std::endl;
            char* response_buffer = nullptr;
            size_t response_buffer_len = 0;
            ret = hako_asset_service_client_get_response(&service_client_handle, &response_buffer, &response_buffer_len, -1);
            if (ret < 0) {
                printf("ERORR: hako_asset_service_client_get_response() returns %d.\n", ret);
                return 1;
            }
            ret = response_convertor.pdu2cpp(response_buffer, response_packet);
            if (ret < 0) {
                printf("ERORR: response_convertor.pdu2cpp() returns %d.\n", ret);
                return 1;
            }
            std::cout << "INFO: hako_asset_service_client_get_response() buffer_len= " << response_buffer_len << std::endl;
            debug_response_packet(response_packet);
            break;
        }
        hako_asset_usleep(delta_time_usec);
        usleep(delta_time_usec);
    }
    return 0;
}

static hako_asset_callbacks_t my_callback = {
    .on_initialize = my_on_initialize,
    .on_manual_timing_control = my_on_manual_timing_control,
    .on_simulation_step = NULL,
    .on_reset = my_on_reset
};
int main(int argc, const char* argv[])
{
    if (argc != 2) {
        printf("Usage: %s <config_path>\n", argv[0]);
        return 1;
    }
    const char* config_path = argv[1];


    int ret = hako_asset_register(asset_name, config_path, &my_callback, delta_time_usec, HAKO_ASSET_MODEL_PLANT);
    if (ret != 0) {
        printf("ERORR: hako_asset_register() returns %d.", ret);
        return 1;
    }
    ret = hako_asset_service_initialize(service_config_path);
    if (ret != 0) {
        printf("ERORR: hako_asset_service_initialize() returns %d.\n", ret);
        return 1;
    }
    ret = hako_asset_start();
    printf("INFO: hako_asset_start() returns %d\n", ret);

    hako_conductor_stop();
    return 0;
}
