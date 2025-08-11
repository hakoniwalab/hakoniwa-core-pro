#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import asyncio
from hakoniwa_pdu.service.shm_common import ShmCommon
from hakoniwa_pdu.service.shm_service_server import ShmServiceServer
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_AddTwoIntsRequest import AddTwoIntsRequest
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_AddTwoIntsResponse import AddTwoIntsResponse
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_conv_AddTwoIntsRequestPacket import py_to_pdu_AddTwoIntsRequestPacket, pdu_to_py_AddTwoIntsRequestPacket
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_conv_AddTwoIntsResponsePacket import py_to_pdu_AddTwoIntsResponsePacket, pdu_to_py_AddTwoIntsResponsePacket

async def my_add_handler(req: AddTwoIntsRequest):
    result = AddTwoIntsResponse()
    result.sum = req.a + req.b
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
    service_server = ShmServiceServer(asset_name, service_name, delta_time_usec, 
                                            py_to_pdu_AddTwoIntsRequestPacket,
                                            pdu_to_py_AddTwoIntsRequestPacket, 
                                            py_to_pdu_AddTwoIntsResponsePacket,
                                            pdu_to_py_AddTwoIntsResponsePacket)
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
