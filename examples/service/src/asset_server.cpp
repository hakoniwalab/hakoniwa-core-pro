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
static const char* asset_name = "Server";
static const char* service_config_path = "./examples/service/service.json";
static const char* service_name = "Service/Add";
static int service_id = -1;
static hako_time_t delta_time_usec = 1000 * 1000;

static int my_on_initialize(hako_asset_context_t* context)
{
    (void)context;
    printf("INFO: hako_asset_service_server_create()...\n");
    service_id = hako_asset_service_server_create(asset_name, service_name);
    if (service_id < 0) {
        printf("ERORR: hako_asset_service_server_create() returns %d.\n", service_id);
        return 1;
    }
    printf("INFO: hako_asset_service_server_create() returns %d.\n", service_id);
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
    HakoCpp_AddTwoIntsResponsePacket response_packet;
    hako::pdu::PduConvertor<HakoCpp_AddTwoIntsRequestPacket, hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket> request_convertor;
    hako::pdu::PduConvertor<HakoCpp_AddTwoIntsResponsePacket, hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket> response_convertor;

    while (true) {
        int ret = hako_asset_service_server_poll(service_id);
        if (ret < 0) {
            printf("ERORR: hako_asset_service_server_poll() returns %d.\n", ret);
            return 1;
        }
        int status = 0;
        int stat_ret = hako_asset_service_server_status(service_id, &status);
        if (stat_ret < 0) {
            printf("ERORR: hako_asset_service_server_status() returns %d.\n", ret);
            return 1;
        }
        std::cout << "INFO: hako_asset_service_server_poll() returns " << ret << " status: " << status << std::endl;
        if (ret == HAKO_SERVICE_SERVER_API_REQUEST_IN) {
            std::cout << "INFO: hako_asset_service_server_get_request()..." << std::endl;
            char* request_buffer = nullptr;
            size_t request_buffer_len = 0;
            ret = hako_asset_service_server_get_request(service_id, &request_buffer, &request_buffer_len);
            if (ret < 0) {
                printf("ERORR: hako_asset_service_server_get_request() returns %d.\n", ret);
                return 1;
            }
            std::cout << "INFO: hako_asset_service_server_get_request() buffer_len= " << request_buffer_len << std::endl;
            ret = request_convertor.pdu2cpp(request_buffer, request_packet);
            if (ret < 0) {
                printf("ERORR: request_convertor.pdu2cpp() returns %d.\n", ret);
                return 1;
            }
            debug_packet(request_packet);
            char* response_buffer = nullptr;
            size_t response_buffer_len = 0;
            ret = hako_asset_service_server_get_response_buffer(service_id, &response_buffer, &response_buffer_len,
                HAKO_SERVICE_API_STATUS_NONE, HAKO_SERVICE_API_RESULT_CODE_OK);
            if (ret < 0) {
                printf("ERORR: hako_asset_service_get_response_buffer() returns %d.\n", ret);
                return 1;
            }
            response_convertor.pdu2cpp(response_buffer, response_packet);
            std::cout << "INFO: hako_asset_service_get_response_buffer() buffer_len= " << response_buffer_len << std::endl;
            response_packet.body.sum = request_packet.body.a + request_packet.body.b;
            debug_response_packet(response_packet);
            ret = response_convertor.cpp2pdu(response_packet, response_buffer, response_buffer_len);
            if (ret < 0) {
                printf("ERORR: response_convertor.cpp2pdu() returns %d.\n", ret);
                return 1;
            }
            
            ret = hako_asset_service_server_put_response(service_id, response_buffer, response_buffer_len);
            if (ret < 0) {
                printf("ERORR: hako_asset_service_server_put_response() returns %d.\n", ret);
                return 1;
            }
            std::cout << "INFO: hako_asset_service_server_put_response() returns " << ret << std::endl;

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

    hako_conductor_start(delta_time_usec, delta_time_usec);
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
