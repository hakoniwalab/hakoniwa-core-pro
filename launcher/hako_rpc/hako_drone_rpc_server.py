from __future__ import annotations
import logging

from .hako_rpc_server import HakoRpcServer
from hako_drone.hako_drone_service import HakoDroneService

# from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_DroneControlRequest import DroneControlRequest
# from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_DroneControlResponse import DroneControlResponse
from hakoniwa_pdu.rpc.codes import SystemControlStatusCode

# 仮の型定義
class DroneControlRequest:
    opcode: int
class DroneControlResponse:
    status_code: int
    message: str

# DroneControlサービス用の定数
DRONE_CONTROL_SERVICE_NAME = "Service/DroneControl"

# DroneControlサービス用のOpCode
DRONE_CMD_TAKEOFF = 1
DRONE_CMD_LAND = 2

class HakoDroneRpcServer(HakoRpcServer):
    def __init__(self, args):
        super().__init__(args)
        self.drone_service = None
        logging.info("HakoDroneRpcServer initialized")

    def _initialize_launcher(self):
        # 親の初期化を呼び出して、SystemControlサービスを登録
        if not super()._initialize_launcher():
            return False
        
        # DroneServiceを初期化
        if self.launcher_service:
            self.drone_service = HakoDroneService(self.launcher_service)
            
            # DroneControlサービスとハンドラを登録
            self.add_service(DRONE_CONTROL_SERVICE_NAME, "DroneControl")
            self.add_handler(DRONE_CONTROL_SERVICE_NAME, self._drone_control_handler)
            return True
        return False

    async def _drone_control_handler(self, req: DroneControlRequest) -> DroneControlResponse:
        res = DroneControlResponse()
        if self.drone_service is None:
            res.status_code = SystemControlStatusCode.FATAL
            res.message = "Drone service not initialized"
            return res
        try:
            match req.opcode:
                case _:
                    res.status_code = SystemControlStatusCode.ERROR
                    res.message = f"Unknown opcode for DroneControl: {req.opcode}"
                    return res
            res.status_code = SystemControlStatusCode.OK
            res.message = "OK"
        except Exception as e:
            res.status_code = SystemControlStatusCode.INTERNAL
            res.message = str(e)
        return res