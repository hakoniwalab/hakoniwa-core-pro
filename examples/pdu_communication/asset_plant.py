#!/usr/bin/python
# -*- coding: utf-8 -*-
import hakopy
import hako_pdu
import pdu_info
import sys
import time

def my_on_initialize(context):
    ret = hakopy.register_data_recv_event("Robot", pdu_info.PDU_MOTOR_CHANNEL_ID)
    print(f"INFO: register_data_recv_event() returns {ret}")
    return 0

def my_on_reset(context):
    return 0

pdu_manager = None
def my_on_manual_timing_control(context):
    global pdu_manager
    print("INFO: on_manual_timing_control enter")
    pdu_pos = pdu_manager.get_pdu('Robot', pdu_info.PDU_POS_CHANNEL_ID)
    pdu_motor = pdu_manager.get_pdu('Robot', pdu_info.PDU_MOTOR_CHANNEL_ID)
    pos = pdu_pos.get()
    result = True
    count = 0
    while result:
        pos['linear']['x'] = count + 1
        pos['linear']['y'] = count + 2
        pos['linear']['z'] = count + 3
        
        ret = pdu_pos.write()
        if ret == False:
            print('"ERROR: hako_asset_pdu_write')
            break
        print(f'{hakopy.simulation_time()}: pos data({pos['linear']['x']}, {pos['linear']['y']}, {pos['linear']['z']})')
        result = hakopy.usleep(1000)
        if result == False:
            break

        ret = hakopy.check_data_recv_event("Robot", pdu_info.PDU_MOTOR_CHANNEL_ID)
        if ret == True:
            print('deta recv event')
            motor = pdu_motor.read()
            if motor == None:
                print('ERROR: hako_asset_pdu_read')
            print(f'{hakopy.simulation_time()}: motor data({motor['linear']['x']}, {motor['linear']['y']}, {motor['linear']['z']})')
        ret = hakopy.check_data_recv_event("Robot", pdu_info.PDU_MOTOR_CHANNEL_ID)
        print(f'second ret: {ret}')

        result = hakopy.usleep(1000)
        if result == False:
            break
        count = count + 1
        time.sleep(1)  # 1秒スリープ
    print("INFO: on_manual_timing_control exit")
    return 0

my_callback = {
    'on_initialize': my_on_initialize,
    'on_simulation_step': None,
    'on_manual_timing_control': my_on_manual_timing_control,
    'on_reset': my_on_reset
}
def main():
    global pdu_manager
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_path>")
        return 1

    asset_name = 'Writer'
    config_path = sys.argv[1]
    delta_time_usec = 1000000

    pdu_manager = hako_pdu.HakoPduManager('/usr/local/lib/hakoniwa/hako_binary/offset', config_path)

    hakopy.conductor_start(delta_time_usec, delta_time_usec)
    ret = hakopy.asset_register(asset_name, config_path, my_callback, delta_time_usec, hakopy.HAKO_ASSET_MODEL_PLANT)
    if ret == False:
        print(f"ERROR: hako_asset_register() returns {ret}.")
        return 1

    ret = hakopy.start()
    print(f"INFO: hako_asset_start() returns {ret}")

    hakopy.conductor_stop()
    return 0

if __name__ == "__main__":
    sys.exit(main())
