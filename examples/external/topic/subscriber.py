#!/usr/bin/env python3
import time
import asyncio
from sources.assets.bindings.python.shm_common import ShmCommon
from sources.assets.bindings.python.shm_topic_subscriber import ShmSubscriber

def on_pos_received(msg):
    print(f"Received in callback: {msg}")

async def main_async():
    service_config_path = "examples/external/topic/service.json"
    pdu_offset_path = './hakoniwa-ros2pdu/pdu/offset'
    delta_time_usec = 1000 * 1000

    shm = ShmCommon(service_config_path, pdu_offset_path, delta_time_usec)
    shm.start_conductor()
    shm.initialize()

    shm.start_service()
    subscriber = ShmSubscriber(shm, "drone2", "pos")
    subscriber.initialize(service_config_path, on_pos_received)

    await subscriber.spin()

    shm.stop_conductor()

def main():
    return asyncio.run(main_async())


if __name__ == "__main__":
    main()
