#!/usr/bin/env python3
import time
import asyncio
from hakoniwa_pdu.service.shm_common import ShmCommon
from hakoniwa_pdu.topic.shm_topic_publisher import ShmPublisher
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_pytype_Twist import Twist
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_conv_Twist import pdu_to_py_Twist, py_to_pdu_Twist

async def main_async():
    service_config_path = "examples/external/topic/service.json"
    pdu_offset_path = './hakoniwa-ros2pdu/pdu/offset'
    delta_time_usec = 1000 * 1000

    shm = ShmCommon(service_config_path, pdu_offset_path, delta_time_usec)
    shm.initialize()

    publisher = ShmPublisher(shm, "drone2", "pos", py_to_pdu_Twist)
    publisher.initialize(service_config_path)

    msg : Twist = Twist()
    count = 0

    while True:
        msg.linear.x = count
        msg.angular.z = count * 0.1
        ret = publisher.publish(msg)
        if ret:
            print(f"Success Published: {msg}")
        else:
            print("Failed to publish message")
        count += 1
        await shm.sleep()

def main():
    return asyncio.run(main_async())


if __name__ == "__main__":
    main()
