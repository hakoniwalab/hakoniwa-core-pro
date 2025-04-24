#!/usr/bin/python
# -*- coding: utf-8 -*-
import hakopy
import hako_pdu
import sys
import time
import sources.assets.bindings.python.hako_asset_service_constants as constants
from sources.assets.bindings.python.hako_asset_service_server import HakoAssetServiceServer

asset_name = 'Server'
service_name = 'Service/Add'
service_config_path = './examples/service/service.json'
service_server = None
delta_time_usec = 1000 * 1000

def hako_sleep(sec: int):
    hakopy.usleep(int(sec * 1000 * 1000))
    time.sleep(sec)
    return 0

def my_on_initialize(context):
    global asset_name
    global service_name
    global service_server
    service_server = HakoAssetServiceServer(pdu_manager, asset_name, service_name)
    if service_server.initialize() == False:
        raise RuntimeError("Failed to create asset service")
    pdu_manager.append_pdu_def(service_config_path)

    return 0

def my_on_reset(context):
    return 0


pdu_manager = None
def my_on_manual_timing_control(context):
    global pdu_manager
    global service_server
    global delta_time_usec
    while True:
        event = service_server.poll()
        if service_server.is_request_in(event):
            # Process the request
            print("Request received")
            request_data = service_server.get_request()
            print(f"Request data: {request_data}")
            res = {
                'sum': request_data['a'] + request_data['b']
            }
            while service_server.normal_reply(res) == False:
                print("INFO: APL normal_reply() is not done")
                hako_sleep(1)
        else:
            hako_sleep(1)

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

    asset_name = 'Server'
    config_path = sys.argv[1]
    delta_time_usec = 1000 * 1000

    pdu_manager = hako_pdu.HakoPduManager('./hakoniwa-ros2pdu/pdu/offset', config_path)

    hakopy.conductor_start(delta_time_usec, delta_time_usec)
    ret = hakopy.asset_register(asset_name, config_path, my_callback, delta_time_usec, hakopy.HAKO_ASSET_MODEL_PLANT)
    if ret == False:
        print(f"ERROR: hako_asset_register() returns {ret}.")
        return 1
    ret = hakopy.service_initialize(service_config_path)
    if ret < 0:
        print(f"ERROR: hako_asset_service_initialize() returns {ret}.")
        return 1
    ret = hakopy.start()
    print(f"INFO: hako_asset_start() returns {ret}")

    hakopy.conductor_stop()
    return 0

if __name__ == "__main__":
    sys.exit(main())
