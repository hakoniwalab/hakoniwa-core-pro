#!/usr/bin/python
# -*- coding: utf-8 -*-
import hakopy
import hako_pdu
import sys
import time
import sources.assets.bindings.python.hako_asset_service_constants as constants
from sources.assets.bindings.python.hako_asset_service_server import HakoAssetServiceServer
import asyncio

asset_name = 'Server'
service_name = 'Service/Add'
service_config_path = './examples/service/service.json'
service_server = None
delta_time_usec = 1000 * 1000
TEST_CASE_NORMAL = 0
TEST_CASE_CANCEL1 = 1
TEST_CASE_CANCEL2 = 2
test_case = TEST_CASE_NORMAL
def hako_sleep(sec: int):
    #print(f"INFO: hako_sleep({sec})")
    ret = hakopy.usleep(int(sec * 1000 * 1000))
    if ret == False:
        sys.exit(1)
    time.sleep(sec)
    #print(f"INFO: hako_sleep({sec}) done")
    return 0

async def hako_sleep_async(sec: int):
    await asyncio.to_thread(hako_sleep, sec)

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

async def normal_test_case():
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
            print(f"INFO: OUT: {res}")
            while service_server.normal_reply(res) == False:
                print("INFO: APL normal_reply() is not done")
                await hako_sleep_async(1)
        else:
            await hako_sleep_async(1)

async def cancel_1_test_case():
    global pdu_manager
    global service_server
    global delta_time_usec
    print("INFO: normal_test_case waiting...")
    await hako_sleep_async(5)
    print("INFO: normal_test_case start")
    event = service_server.poll()
    if event < 0:
        print(f"ERROR: hako_asset_service_server_poll() returns {event}")
        return 1
    if service_server.is_request_in(event):
        req = service_server.get_request()
        print(f"Request data: {req}")
        res = {
            'sum': req['a'] + req['b']
        }
        await hako_sleep_async(5)
        event = service_server.poll()
        print(f"event: {event}")
        if service_server.is_request_cancel(event):
            print("Request cancelled")
            service_server.cancel_reply(res)
        else:
            print("Request not cancelled")
            service_server.normal_reply(res)

    return 0

async def cancel_2_test_case():
    global pdu_manager
    global service_server
    global delta_time_usec
    print("INFO: normal_test_case waiting...")
    await hako_sleep_async(5)
    print("INFO: normal_test_case start")
    event = service_server.poll()
    if event < 0:
        print(f"ERROR: hako_asset_service_server_poll() returns {event}")
        return 1
    if service_server.is_request_in(event):
        req = service_server.get_request()
        print(f"Request data: {req}")
        res = {
            'sum': req['a'] + req['b']
        }
        await hako_sleep_async(5)
        print(f"INFO: OUT: {res}")
        service_server.normal_reply(res)
        event = service_server.poll()
        print(f"event: {event}")

    return 0

pdu_manager = None
def my_on_manual_timing_control(context):
    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(run_client_task())
    except KeyboardInterrupt:
        print("Interrupted by user.")
    return 0

async def run_client_task():
    global test_case
    if test_case == TEST_CASE_NORMAL:
        await normal_test_case()
    elif test_case == TEST_CASE_CANCEL1:
        await cancel_1_test_case()
    elif test_case == TEST_CASE_CANCEL2:
        await cancel_2_test_case()

    while True:
        await hako_sleep_async(1)

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
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <config_path>  <test_case: normal | cancel1 | cancel2>")
        return 1

    asset_name = 'Server'
    config_path = sys.argv[1]
    if sys.argv[2] == 'normal':
        test_case = TEST_CASE_NORMAL
    elif sys.argv[2] == 'cancel1':
        test_case = TEST_CASE_CANCEL1
    elif sys.argv[2] == 'cancel2':
        test_case = TEST_CASE_CANCEL2
    else:
        print(f"ERROR: invalid test case {sys.argv[2]}")
        return 1
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
