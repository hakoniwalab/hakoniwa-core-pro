#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_conductor.h"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"
#include "pdu_convertor.hpp"
#include "hako_asset_service_server.hpp"

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
static hako_time_t delta_time_usec = 1000 * 1000;


HakoAssetServiceServerTemplateType(AddTwoInts) service_server(asset_name, service_name);

static int my_on_initialize(hako_asset_context_t* context)
{
    (void)context;
    int ret = service_server.initialize();
    if (ret < 0) {
        std::cout << "ERROR: service_server.initialize() returns " << ret << std::endl;
        return 1;
    }
    //std::cout << "INFO: service_server.initialize() returns " << ret << std::endl;
    return 0;
}

static int my_on_manual_timing_control(hako_asset_context_t* context)
{
    (void)context;
    std::cout << "*************** START SERVICE SERVER ***************" << std::endl;
    while (true) {
        hako_asset_usleep(5* delta_time_usec);
        usleep(5* delta_time_usec);

        int ret = service_server.poll();
        if (ret < 0) {
            printf("ERORR: hako_asset_service_server_poll() returns %d.\n", ret);
            return 1;
        }
        if (ret == HAKO_SERVICE_SERVER_API_REQUEST_IN) {
            HakoCpp_AddTwoIntsRequest req = service_server.get_request();
            HakoCpp_AddTwoIntsResponse res = {};
            res.sum = req.a + req.b;
            std::cout << "IN: a=" << req.a << " b=" << req.b << std::endl;
            std::cout << "OUT: sum=" << res.sum << std::endl;
            (void)service_server.reply(res);
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
