#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import asyncio
from sources.assets.bindings.python.shm_common import ShmCommon
from sources.assets.bindings.python.shm_service_server import ShmServiceServer

async def my_add_handler(req):
    result = {'sum': req['a'] + req['b']}
    return result

async def main_async():

    asset_name = None
    service_name = 'Service/Add'
    service_config_path = './examples/external/service/service.json'
    pdu_offset_path = './hakoniwa-ros2pdu/pdu/offset'
    delta_time_usec = 1000 * 1000

    shm = ShmCommon(service_config_path, pdu_offset_path, delta_time_usec)
    shm.start_conductor()
    if shm.initialize() == False:
        print("Failed to initialize shm")
        return 1
    shm.start_service()
    service_server = ShmServiceServer(asset_name, service_name, delta_time_usec)
    if service_server.initialize(shm) == False:
        print("Failed to initialize service client")
        return 1

    await service_server.serve(my_add_handler)
    shm.stop_conductor()
    return 0

def main():
    return asyncio.run(main_async())

if __name__ == "__main__":
    sys.exit(main())
