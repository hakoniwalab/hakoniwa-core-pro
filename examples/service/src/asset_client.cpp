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

hako_time_t delta_time_usec = 1000 * 1000;
static void hako_sleep()
{
    hako_asset_usleep(delta_time_usec);
    usleep(delta_time_usec);
}

HakoAssetServiceClientTemplateType(AddTwoInts) service_client(asset_name, "Service/Add", "Client01");

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
    bool is_timeout_happened = false;
    std::cout << "*************** START SERVICE CLIENT ***************" << std::endl;
    // 1. request
    HakoCpp_AddTwoIntsRequest req = {};
    req.a = 1;
    req.b = 2;
    HakoCpp_AddTwoIntsResponse res = {};
    res.sum = 0;
    std::cout << "INFO: APL request: a=" << req.a << " b=" << req.b << std::endl;
    std::cout << "INFO: APL request..." << std::endl;
    int ret = service_client.request(req, 2000, -1);
    if (ret < 0) {
        printf("ERORR: service_client.request() returns %d.\n", ret);
        return 1;
    }
    std::cout << "INFO: APL wait for response " << ret << std::endl;
    // 2. poll
    while (true) {
        hako_sleep();
        ret = service_client.poll();
        if (ret < 0) {
            printf("ERORR: service_client.poll() returns %d.\n", ret);
            return 1;
        }
        if (service_client.is_response_in(ret)) {
            res = service_client.get_response();
            std::cout << "OUT: sum=" << res.sum << std::endl;
            break;
        }
        if (service_client.is_request_timeout(ret)) {
            is_timeout_happened = true;
            break;
        }
    }
    // 3. if request_timeout, then cancel request
    if (is_timeout_happened) {
        printf("WARNING: APL cancel request is happened.\n");
        while (service_client.cancel_request() == false) {
            hako_sleep();
            std::cout << "INFO: APL cancel_request() is not done." << std::endl;
        }
        std::cout << "INFO: APL cancel_request() is done." << std::endl;
        while (true) {
            ret = service_client.poll();
            if (service_client.is_request_cancel_done(ret)) {
                printf("INFO: APL cancel request is done.\n");
                break;
            }
            else if (service_client.is_response_in(ret)) {
                res = service_client.get_response();
                std::cout << "OUT: sum=" << res.sum << std::endl;
                break;
            }
            else {
                printf("ERORR: APL cancel request is not done.\n");
            }
            hako_sleep();
        }
    }
    // finish
    std::cout << "*************** FINISH SERVICE CLIENT ***************" << std::endl;
    while (true) {
        hako_sleep();
    }
    
    return 0;
}
static int my_on_reset(hako_asset_context_t* context)
{
    (void)context;
    return 0;
}


static hako_asset_callbacks_t my_callback;
int main(int argc, const char* argv[])
{
    my_callback.on_initialize = my_on_initialize;
    my_callback.on_manual_timing_control = my_on_manual_timing_control;
    my_callback.on_reset = my_on_reset;
    my_callback.on_simulation_step = NULL;
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
