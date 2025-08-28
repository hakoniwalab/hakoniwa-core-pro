from __future__ import annotations
import logging
from .hako_rpc_server import HakoRpcServer

class HakoDroneRpcServer(HakoRpcServer):
    def __init__(self, args):
        super().__init__(args)
        logging.info("HakoDroneRpcServer initialized")

    # NOTE: ドローン用のRPCハンドラやメソッドをここに追加・上書きしてください。
    # 例:
    # def _initialize_launcher(self):
    #     logging.info("Drone server initializing launcher...")
    #     # ドローン特有のLauncherServiceサブクラスを使うなど
    #     return super()._initialize_launcher()