#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import asyncio
from sources.assets.bindings.python.shm_common import ShmCommon
from sources.assets.bindings.python.shm_service_client import ShmServiceClient

async def main_async():

    asset_name = None
    service_name = 'Service/Add'
    service_config_path = './examples/external/service/service.json'
    pdu_offset_path = './hakoniwa-ros2pdu/pdu/offset'
    service_client = None
    delta_time_usec = 1000 * 1000

    shm = ShmCommon(service_config_path, pdu_offset_path, delta_time_usec)
    if shm.initialize() == False:
        print("Failed to initialize shm")
        return 1
    service_client = ShmServiceClient(asset_name, service_name, "Client01", delta_time_usec)
    if service_client.initialize(shm) == False:
        print("Failed to initialize service client")
        return 1

    req = service_client.get_empty_request()
    req['a'] = 1
    req['b'] = 2

    res = await service_client.call_async(req)
    if res is None:
        print("Failed to get response")
        return 1
    print(f"Response: {res}")
    return 0

def main():
    return asyncio.run(main_async())

if __name__ == "__main__":
    sys.exit(main())
