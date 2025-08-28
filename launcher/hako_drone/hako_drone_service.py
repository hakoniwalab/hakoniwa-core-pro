from __future__ import annotations
import logging
from hako_launch.hako_launcher import LauncherService

class HakoDroneService:
    def __init__(self, launcher_service: LauncherService):
        self.launcher_service = launcher_service
        logging.info("HakoDroneService initialized")

    def takeoff(self):
        """ドローンを離陸させる"""
        logging.info("CMD: takeoff")
        # ここにドローンへの離陸コマンド送信処理を実装します
        pass

    def land(self):
        """ドローンを着陸させる"""
        logging.info("CMD: land")
        # ここにドローンへの着陸コマンド送信処理を実装します
        pass

    def get_telemetry(self):
        """ドローンの状態を取得する"""
        logging.info("CMD: get_telemetry")
        # ここにドローンの状態取得処理を実装します
        return {}
