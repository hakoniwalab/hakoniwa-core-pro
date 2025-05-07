#!/usr/bin/env python3
import time
import asyncio
from sources.assets.bindings.python.shm_common import ShmCommon
from sources.assets.bindings.python.shm_topic_publisher import ShmPublisher


async def main_async():
    service_config_path = "examples/external/topic/service.json"
    pdu_offset_path = './hakoniwa-ros2pdu/pdu/offset'
    delta_time_usec = 1000 * 1000

    shm = ShmCommon(service_config_path, pdu_offset_path, delta_time_usec)
    shm.initialize()

    publisher = ShmPublisher(shm, "drone2", "pos")
    publisher.initialize(service_config_path)

    msg = publisher.get_empty_message()
    count = 0

    while True:
        msg['linear']['x'] = count
        msg['angular']['z'] = count * 0.1
        publisher.publish(msg)
        print(f"Published: {msg}")
        count += 1
        await shm.sleep()

def main():
    return asyncio.run(main_async())


if __name__ == "__main__":
    main()
