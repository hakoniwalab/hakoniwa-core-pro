#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_conductor.h"
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

static int my_on_manual_timing_control(hako_asset_context_t* context)
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
    hako_time_t delta_time_usec = 1000;


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
