#include "hako_asset.h"
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

static int my_on_initialize(hako_asset_context_t* context)
{
    (void)context;
    return 0;
}
static int my_on_reset(hako_asset_context_t* context)
{
    (void)context;
    return 0;
}

static void on_recv()
{
    printf("INFO: on_recv\n");
    Hako_Twist motor;
    int ret = hako_asset_pdu_read("Robot", PDU_MOTOR_CHANNEL_ID, (char*)(&motor), sizeof(motor));
    if (ret != 0) {
        printf("ERROR: hako_asset_pdu_read erro: %d\n", ret);
    }
    printf("%llu: motor data(%f, %f, %f)\n", hako_asset_simulation_time(), motor.linear.x, motor.linear.y, motor.linear.z);
}

static int my_on_manual_timing_control(hako_asset_context_t* context)
{
    (void)context;
    const char* robot_name = "Robot";
    int ret = hako_asset_register_data_recv_event(robot_name, PDU_MOTOR_CHANNEL_ID, on_recv);
    if (ret != 0) {
        printf("ERORR: hako_asset_register_data_recv_event() returns %d.", ret);
        return 1;
    }

    Hako_Twist pos;
    printf("INFO: on_manual_timing_control enter\n");
    int result = 0;
    double count = 0;
    while (result == 0) {
        pos.linear.x = count + 1;
        pos.linear.y = count + 2;
        pos.linear.z = count + 3;
        int ret = hako_asset_pdu_write("Robot", PDU_POS_CHANNEL_ID, (const char*)(&pos), sizeof(pos));
        if (ret != 0) {
            printf("ERROR: hako_asset_pdu_read erro: %d\n", ret);
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
    const char* asset_name = "Writer";
    const char* config_path = argv[1];
    hako_time_t delta_time_usec = 1000000;

    hako_conductor_start(delta_time_usec, delta_time_usec);
    int ret = hako_asset_register(asset_name, config_path, &my_callback, delta_time_usec, HAKO_ASSET_MODEL_PLANT);
    if (ret != 0) {
        printf("ERORR: hako_asset_register() returns %d.", ret);
        return 1;
    }
    ret = hako_asset_start();
    printf("INFO: hako_asset_start() returns %d\n", ret);

    hako_conductor_stop();
    return 0;
}
