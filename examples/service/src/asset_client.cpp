#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_conductor.h"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"
#include "hako_asset_service_client.hpp"
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

hako_time_t delta_time_usec = 1000 * 1000;
hako::service::HakoAssetServiceClient<
    HakoCpp_AddTwoIntsRequestPacket, 
    HakoCpp_AddTwoIntsResponsePacket,
    HakoCpp_AddTwoIntsRequest, 
    HakoCpp_AddTwoIntsResponse,
    hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket,
    hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket> 
    service_client(asset_name, service_name, service_client_name);

static int my_on_initialize(hako_asset_context_t* context)
{
    (void)context;
    std::cout << "INFO: my_on_initialize()..." << std::endl;
    int ret = service_client.initialize();
    if (ret < 0) {
        std::cout << "ERROR: service_client.initialize() returns " << ret << std::endl;
        return 1;
    }
    std::cout << "INFO: service_client.initialize() returns " << ret << std::endl;
    return 0;
}
static int my_on_manual_timing_control(hako_asset_context_t* context)
{
    (void)context;
    while (true) {
        int ret = service_client.poll();
        if (ret < 0) {
            printf("ERORR: hako_asset_service_client_poll() returns %d.\n", ret);
            return 1;
        }
        if ((service_client.status() == HAKO_SERVICE_CLIENT_API_STATE_IDLE) && (ret == HAKO_SERVICE_CLIENT_API_EVENT_NONE))
        {
            HakoCpp_AddTwoIntsRequest req = {};
            req.a = 1;
            req.b = 2;
            std::cout << "IN: a=" << req.a << ", b=" << req.b << std::endl;
            int ret = service_client.request(req);
            if (ret < 0) {
                printf("ERORR: service_client.request() returns %d.\n", ret);
                return 1;
            }
        }
        else if (ret == HAKO_SERVICE_CLIENT_API_RESPONSE_IN) {
            HakoCpp_AddTwoIntsResponse res = service_client.get_response();
            std::cout << "OUT: sum=" << res.sum << std::endl;
            break;
        }
        else if (ret == HAKO_SERVICE_CLIENT_API_EVENT_NONE) {
            // Do nothing
        }
        else {
            printf("ERORR: hako_asset_service_client_poll() returns %d.\n", ret);
            return 1;

        }
        hako_asset_usleep(delta_time_usec);
        usleep(delta_time_usec);
    }
    return 0;
}
static int my_on_reset(hako_asset_context_t* context)
{
    (void)context;
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
