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
static void hako_sleep()
{
    hako_asset_usleep(5 * delta_time_usec);
    usleep(5 * delta_time_usec);
}
static void hako_sleep1()
{
    hako_asset_usleep(delta_time_usec);
    usleep(delta_time_usec);
}

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
enum class TestCase {
    NORMAL,
    CANCEL_1,
    CANCEL_2,
};
static TestCase test_case = TestCase::NORMAL;
static void cancel_1_test_case()
{
    std::cout << "INFO: wait for while...";
    hako_sleep();
    std::cout << "INFO: APL poll..." << std::endl;
    int ret = service_server.poll();
    if (ret < 0) {
        printf("ERORR: hako_asset_service_server_poll() returns %d.\n", ret);
        return;
    }
    if (service_server.is_request_in(ret)) {
        HakoCpp_AddTwoIntsRequest req = service_server.get_request();
        HakoCpp_AddTwoIntsResponse res = {};
        res.sum = req.a + req.b;
        std::cout << "IN: a=" << req.a << " b=" << req.b << std::endl;
        hako_sleep();
        int ret = service_server.poll();
        std::cout << "INFO: APL EVENT: " << ret << std::endl;
        if (service_server.is_request_cancel(ret)) {
            printf("WARNING: APL cancel request is happened.\n");
            service_server.cancel_reply(res);
        }
        else {
            std::cout << "OUT: sum=" << res.sum << std::endl;
            (void)service_server.normal_reply(res);
        }
    }
}
static void cancel_2_test_case()
{
    std::cout << "INFO: wait for while...";
    hako_sleep();
    std::cout << "INFO: APL poll..." << std::endl;
    int ret = service_server.poll();
    if (ret < 0) {
        printf("ERORR: hako_asset_service_server_poll() returns %d.\n", ret);
        return;
    }
    if (service_server.is_request_in(ret)) {
        HakoCpp_AddTwoIntsRequest req = service_server.get_request();
        HakoCpp_AddTwoIntsResponse res = {};
        res.sum = req.a + req.b;
        std::cout << "IN: a=" << req.a << " b=" << req.b << std::endl;
        hako_sleep();
        std::cout << "OUT: sum=" << res.sum << std::endl;
        (void)service_server.normal_reply(res);
        int ret = service_server.poll();
        std::cout << "INFO: APL EVENT: " << ret << std::endl;
    }
}
static void normal_test_case()
{
    std::cout << "INFO: APL poll..." << std::endl;
    while (true) {
        int ret = service_server.poll();
        if (ret < 0) {
            printf("ERORR: hako_asset_service_server_poll() returns %d.\n", ret);
            return;
        }
        if (service_server.is_request_in(ret)) {
            HakoCpp_AddTwoIntsRequest req = service_server.get_request();
            HakoCpp_AddTwoIntsResponse res = {};
            res.sum = req.a + req.b;
            std::cout << "IN: a=" << req.a << " b=" << req.b << std::endl;
            std::cout << "OUT: sum=" << res.sum << std::endl;
            while (service_server.normal_reply(res) == false) {
                hako_sleep1();
                std::cout << "INFO: APL normal_reply() is not done." << std::endl;
            }
            break;
        }
        else {
            printf("ERORR: APL is not request_in.\n");
        }
        hako_sleep1();
    }
}

static int my_on_manual_timing_control(hako_asset_context_t* context)
{
    (void)context;
    std::cout << "*************** START SERVICE SERVER ***************" << std::endl;
    switch (test_case) {
        case TestCase::NORMAL:
            normal_test_case();
            break;
        case TestCase::CANCEL_1:
            cancel_1_test_case();
            break;
        case TestCase::CANCEL_2:
            cancel_2_test_case();
            break;
    }
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

static hako_asset_callbacks_t my_callback = {
    .on_initialize = my_on_initialize,
    .on_manual_timing_control = my_on_manual_timing_control,
    .on_simulation_step = NULL,
    .on_reset = my_on_reset
};
int main(int argc, const char* argv[])
{
    if (argc < 3) {
        printf("Usage: %s <config_path> <test_case: normal | cancel1 | cancel2>\n", argv[0]);
        return 1;
    }
    const char* config_path = argv[1];
    const char* case_name = argv[2];

    if (strcmp(case_name, "normal") == 0) {
        test_case = TestCase::NORMAL;
    } else if (strcmp(case_name, "cancel1") == 0) {
        test_case = TestCase::CANCEL_1;
    } else if (strcmp(case_name, "cancel2") == 0) {
        test_case = TestCase::CANCEL_2;
    } else {
        printf("ERROR: Unknown test case: %s\n", case_name);
        return 1;
    }
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
