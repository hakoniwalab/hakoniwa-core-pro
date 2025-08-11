#!/usr/bin/python
# -*- coding: utf-8 -*-

import hakopy
from hakoniwa_pdu.pdu_manager import PduManager
from hakoniwa_pdu.impl.shm_communication_service import ShmCommunicationService
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_pytype_Twist import Twist
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_conv_Twist import pdu_to_py_Twist, py_to_pdu_Twist
import pdu_info
import sys
import time

global pdu_manager
def on_recv(recv_event_id):
    global pdu_manager
    pdu_manager.run_nowait()
    print(f"INFO: on_recv {recv_event_id} enter")
    try:
        raw_data: bytes = pdu_manager.read_pdu_raw_data('Robot', 'pos')
        if raw_data is None or len(raw_data) == 0:
            print('ERROR: can not read pos data')
            return 0
        pos: Twist = pdu_to_py_Twist(raw_data)
        if pos == None:
            print('ERROR: hako_asset_pdu_read')
        else:
            print(f'{hakopy.simulation_time()}: READ pos data({pos.linear.x}, {pos.linear.y}, {pos.linear.z})')
    except Exception as e:
        print(f"ERROR: {e}")
    print("INFO: on_recv exit")
    return 0

def my_on_initialize(context):
    ret = hakopy.register_data_recv_event("Robot", pdu_info.PDU_POS_CHANNEL_ID, on_recv)
    print(f"INFO: register_data_recv_event() returns {ret}")
    return 0

def my_on_reset(context):
    return 0

pdu_manager = None
def my_on_manual_timing_control(context):
    global pdu_manager
    print("INFO: on_manual_timing_control enter")
    motor: Twist = Twist()
    result = True
    count = 0
    while result:
        motor.linear.x = count + 1001
        motor.linear.y = count + 1002
        motor.linear.z = count + 1003

        raw_data: bytes = py_to_pdu_Twist(motor)
        ret = pdu_manager.flush_pdu_raw_data_nowait('Robot', 'motor', raw_data)
        if ret == False:
            print('ERROR: hako_asset_pdu_write')
            break
        print(f'{hakopy.simulation_time()}: WRITE motor data({motor.linear.x}, {motor.linear.y}, {motor.linear.z})')
        result = hakopy.usleep(1000)
        time.sleep(1)
        if result == False:
            break

        count = count + 1
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

    asset_name = 'Reader'
    config_path = sys.argv[1]
    delta_time_usec = 1000

    pdu_manager = PduManager()
    pdu_manager.initialize(config_path=config_path, comm_service=ShmCommunicationService())
    pdu_manager.start_service_nowait()

    ret = hakopy.asset_register(asset_name, config_path, my_callback, delta_time_usec, hakopy.HAKO_ASSET_MODEL_CONTROLLER)
    if ret == False:
        print(f"ERROR: hako_asset_register() returns {ret}.")
        return 1

    ret = hakopy.start()
    print(f"INFO: hako_asset_start() returns {ret}")

    return 0

if __name__ == "__main__":
    sys.exit(main())
