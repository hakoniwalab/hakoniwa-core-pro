#!/usr/bin/python
# -*- coding: utf-8 -*-
import hakopy
import hako_pdu
import sys
import time
import sources.assets.bindings.python.hako_asset_service_constants as constants
from sources.assets.bindings.python.hako_asset_service_client import HakoAssetServiceClient


asset_name = 'Client'
service_name = 'Service/Add'
service_config_path = './examples/service/service.json'
service_client = None
delta_time_usec = 1000 * 1000

def hako_sleep(sec: int):
    hakopy.usleep(int(sec * 1000 * 1000))
    time.sleep(sec)
    return 0

def my_on_initialize(context):
    global asset_name
    global service_name
    global service_client
    service_client = HakoAssetServiceClient(pdu_manager, asset_name, service_name, "Client01")
    if service_client.initialize() == False:
        raise RuntimeError("Failed to create asset service")
    pdu_manager.append_pdu_def(service_config_path)

    return 0

def my_on_reset(context):
    return 0


pdu_manager = None
def my_on_manual_timing_control(context):
    global pdu_manager
    global service_client
    global delta_time_usec
    is_timeout_heppend = False
    print("*************** START SERVICE CLIENT ***************")
    req = {
        'a': 1,
        'b': 2
    }
    res = {}
    print(f"Request data: {req}")
    result = service_client.request(req, 2000, -1)
    if result == False:
        print("ERROR: Failed to send request")
        return -1
    print("INFO: APL wait for response ")

    while True:
        hako_sleep(1)
        event = service_client.poll()
        if event < 0:
            print(f"ERROR: Failed to poll asset service client: {event}")
            return -1
        if service_client.is_response_in(event):
            res = service_client.get_response()
            print(f"Response data: {res}")
            break
        if service_client.is_request_timeout(event):
            print("INFO: Request timeout")
            is_timeout_heppend = True
            break
    if is_timeout_heppend:
        print("WARNING: APL cancel request is happened.")
        while service_client.cancel_request() == False:
            print("INFO: APL cancel_request() is not done")
            hako_sleep(1)
        while True:
            event = service_client.poll()
            if event < 0:
                print(f"ERROR: Failed to poll asset service client: {event}")
                return -1
            if service_client.is_request_cancel_done(event):
                print("INFO: Request cancel done")
                break
            elif service_client.is_response_in(event):
                res = service_client.get_response()
                print(f"Response data: {res}")
                break
            else:
                print("INFO: Request cancel is not done")
            hako_sleep(1)

    print("*************** FINISH SERVICE CLIENT ***************")
    while True:
        hako_sleep(1)

    return 0

my_callback = {
    'on_initialize': my_on_initialize,
    'on_simulation_step': None,
    'on_manual_timing_control': my_on_manual_timing_control,
    'on_reset': my_on_reset
}
def main():
    global test_case
    global pdu_manager
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_path>")
        return 1

    config_path = sys.argv[1]
    delta_time_usec = 1000 * 1000

    pdu_manager = hako_pdu.HakoPduManager('./hakoniwa-ros2pdu/pdu/offset', config_path)

    ret = hakopy.asset_register(asset_name, config_path, my_callback, delta_time_usec, hakopy.HAKO_ASSET_MODEL_CONTROLLER)
    if ret == False:
        print(f"ERROR: hako_asset_register() returns {ret}.")
        return 1
    ret = hakopy.service_initialize(service_config_path)
    if ret < 0:
        print(f"ERROR: hako_asset_service_initialize() returns {ret}.")
        return 1
    ret = hakopy.start()
    print(f"INFO: hako_asset_start() returns {ret}")

    return 0

if __name__ == "__main__":
    sys.exit(main())
