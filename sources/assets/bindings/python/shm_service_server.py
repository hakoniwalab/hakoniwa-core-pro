import asyncio
from hako_asset_service_server import HakoAssetServiceServer
from shm_common import ShmCommon

class ShmServiceServer:
    def __init__(self, asset_name: str, service_name: str, delta_time_usec: int = 1000):
        self.asset_name = asset_name
        self.service_name = service_name
        self.delta_time_usec = float(delta_time_usec)
        self.service_server = None

    def initialize(self, shm: ShmCommon):
        self.service_server = HakoAssetServiceServer(shm.pdu_manager, self.asset_name, self.service_name)
        shm.pdu_manager.append_pdu_def(shm.service_config_path)
        self.shm = shm
        if not self.service_server.initialize():
            raise RuntimeError("Failed to create asset service server")
        print(f"Service server initialized for {self.service_name}")
        return True


    async def serve(self, handler):
        while True:
            event = self.service_server.poll()
            if event < 0:
                raise RuntimeError(f"Failed to poll asset service server: {event}")
            elif self.service_server.is_request_in(event):
                req = self.service_server.get_request()
                print(f"Request received: {req}")
                res = await handler(req)
                print(f"Response data: {res}")
                while not self.service_server.normal_reply(res):
                    print("Waiting to send reply...")
                    await self.shm.sleep()
            else:
                await self.shm.sleep()
