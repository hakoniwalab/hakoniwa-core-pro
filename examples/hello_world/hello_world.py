import pro.hakopy as hakopy
import sys
import time

def my_on_initialize(context):
    print("INFO: my_on_initialize enter")
    print("INFO: sleep 1sec")
    time.sleep(1)  # 1秒スリープ
    print("INFO: my_on_initialize exit")
    return 0

def my_on_reset(context):
    print("INFO: my_on_reset enter")
    print("INFO: sleep 1sec")
    time.sleep(1)  # 1秒スリープ
    print("INFO: my_on_reset exit")
    return 0

def my_on_simulation_step(context):
    print(f"INFO: on_simulation_step enter: {hakopy.simulation_time()}")
    print("INFO: sleep 1sec")
    time.sleep(1)  # 1秒スリープ
    print("INFO: on_simulation_step exit")
    return 0

def my_on_manual_timing_control(context):
    print("INFO: on_manual_timing_control enter")
    result = 0
    while result == 0:
        print(f"INFO: sleep 1sec: {hakopy.simulation_time()}")
        result = hakopy.usleep(1000) 
        time.sleep(1)  # 1秒スリープ
    print("INFO: on_manual_timing_control exit")
    return 0

my_callback = {
    'on_initialize': my_on_initialize,
    'on_simulation_step': None,
    'on_manual_timing_control': None,
    'on_reset': my_on_reset
}
def main():
    if len(sys.argv) != 4 and len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <asset_name> <config_path> <delta_time_msec> [manual]")
        return 1

    asset_name = sys.argv[1]
    config_path = sys.argv[2]
    delta_time_usec = int(sys.argv[3]) * 1000

    if len(sys.argv) == 4:
        my_callback['on_simulation_step'] = my_on_simulation_step
    else:
        my_callback['on_manual_timing_control'] = my_on_manual_timing_control


    hakopy.conductor_start(delta_time_usec, delta_time_usec)
    ret = hakopy.asset_register(asset_name, config_path, my_callback, delta_time_usec, hakopy.HAKO_ASSET_MODEL_CONTROLLER)
    if ret == False:
        print(f"ERROR: hako_asset_register() returns {ret}.")
        return 1

    ret = hakopy.start()
    print(f"INFO: hako_asset_start() returns {ret}")

    hakopy.conductor_stop()
    return 0

if __name__ == "__main__":
    sys.exit(main())
