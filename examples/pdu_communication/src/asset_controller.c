#include "hako_asset.h"
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

static const char* robot_name = "Robot";
static int my_on_initialize(hako_asset_context_t* context)
{
    (void)context;
    int ret = hako_asset_register_data_recv_event(robot_name, PDU_POS_CHANNEL_ID, NULL);
    if (ret != 0) {
        printf("ERORR: hako_asset_register_data_recv_event() returns %d.", ret);
        return 1;
    }
    return 0;
}
static int my_on_reset(hako_asset_context_t* context)
{
    (void)context;
    return 0;
}

static void on_recv()
{
    Hako_Twist pos;
    printf("INFO: on_recv\n");
    int ret = hako_asset_pdu_read("Robot", PDU_POS_CHANNEL_ID, (char*)(&pos), sizeof(pos));
    if (ret != 0) {
        printf("ERROR: hako_asset_pdu_read erro: %d\n", ret);
    }
    printf("%llu: pos data(%f, %f, %f)\n", hako_asset_simulation_time(), pos.linear.x, pos.linear.y, pos.linear.z);
}

static int my_on_manual_timing_control(hako_asset_context_t* context)
{
    (void)context;
    Hako_Twist motor;
    printf("INFO: on_manual_timing_control enter\n");
    int result = 0;
    double count = 0;
    while (result == 0) {
        motor.linear.x = count + 1001;
        motor.linear.y = count + 1002;
        motor.linear.z = count + 1003;
        int ret = hako_asset_pdu_write("Robot", PDU_MOTOR_CHANNEL_ID, (const char*)(&motor), sizeof(motor));
        if (ret != 0) {
            printf("ERROR: hako_asset_pdu_write erro: %d\n", ret);
        }
        ret = hako_asset_check_data_recv_event(robot_name, PDU_POS_CHANNEL_ID);
        if (ret == 0) {
            on_recv();
        }
        result = hako_asset_usleep(1000);
        if (result != 0) {
            break;
        }
        result = hako_asset_usleep(1000);
        usleep(1000*1000);
        count++;
    }
    printf("INFO: on_manual_timing_control exit\n");
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
    const char* asset_name = "Reader";
    const char* config_path = argv[1];
    hako_time_t delta_time_usec = 1000000;

    int ret = hako_asset_register(asset_name, config_path, &my_callback, delta_time_usec, HAKO_ASSET_MODEL_CONTROLLER);
    if (ret != 0) {
        printf("ERORR: hako_asset_register() returns %d.", ret);
        return 1;
    }
    ret = hako_asset_start();
    printf("INFO: hako_asset_start() returns %d\n", ret);

    return 0;
}
