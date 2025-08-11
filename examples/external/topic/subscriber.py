#!/usr/bin/env python3
import time
import asyncio
from hakoniwa_pdu.service.shm_common import ShmCommon
from hakoniwa_pdu.topic.shm_topic_subscriber import ShmSubscriber
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_pytype_Twist import Twist
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_conv_Twist import pdu_to_py_Twist, py_to_pdu_Twist

def on_pos_received(msg: Twist):
    print(f"Received in callback: {msg}")

async def main_async():
    service_config_path = "examples/external/topic/service.json"
    pdu_offset_path = './hakoniwa-ros2pdu/pdu/offset'
    delta_time_usec = 1000 * 1000

    shm = ShmCommon(service_config_path, pdu_offset_path, delta_time_usec)
    shm.start_conductor()
    shm.initialize()

    shm.start_service()
    subscriber = ShmSubscriber(shm, "drone2", "pos", pdu_to_py_Twist)
    subscriber.initialize(service_config_path, on_pos_received)

    await subscriber.spin()

    shm.stop_conductor()

def main():
    return asyncio.run(main_async())


if __name__ == "__main__":
    main()
