from __future__ import annotations
import logging
import asyncio
import signal
import sys

from hakoniwa_pdu.rpc.codes import SystemControlOpCode, SystemControlStatusCode
from hakoniwa_pdu.impl.websocket_server_communication_service import (
    WebSocketServerCommunicationService,
)
from hakoniwa_pdu.rpc.remote.remote_pdu_service_server_manager import (
    RemotePduServiceServerManager,
)
from hakoniwa_pdu.rpc.auto_wire import make_protocol_server
from hakoniwa_pdu.rpc.protocol_server import ProtocolServerBlocking
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_SystemControlRequest import (
    SystemControlRequest,
)
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_SystemControlResponse import (
    SystemControlResponse,
)
from hakoniwa_pdu.rpc.service_config import patch_service_base_size

from hako_launch.hako_launcher import LauncherService

# 定数
ASSET_NAME = "HakoRpcBaseServer"
SERVICE_NAME = "Service/SystemControl"
OFFSET_PATH = "/Users/tmori/project/oss/hakoniwa-pdu-python/tests/config/offset"
DELTA_TIME_USEC = 1_000_000

def _install_sigint(service: LauncherService):
    def _sigint_handler(signum, frame):
        print("[rpc_server] SIGINT received -> aborting...", file=sys.stderr)
        try:
            service.terminate()
        finally:
            sys.exit(1)
    signal.signal(signal.SIGINT, _sigint_handler)

class HakoRpcServer:
    def __init__(self, args):
        self.args = args
        self.launcher_service = None

    def _initialize_launcher(self):
        try:
            self.launcher_service = LauncherService(launch_path=self.args.launch_file)
            _install_sigint(self.launcher_service)
            logging.info("HakoRPCServer ready. assets:")
            for a in self.launcher_service.spec.assets:
                logging.info(f" - {a.name} (cwd={a.cwd}, cmd={a.command}, args={a.args})")
            return True
        except Exception as e:
            logging.error(f"Failed to load spec: {e}")
            return False

    async def start(self):
        """RPCサーバーを起動し、リクエストを待ち受けます。"""
        if not self._initialize_launcher():
            return

        comm = WebSocketServerCommunicationService(version="v2")
        manager = RemotePduServiceServerManager(
            asset_name=ASSET_NAME,
            pdu_config_path=self.args.pdu_config,
            offset_path=OFFSET_PATH,
            comm_service=comm,
            uri=self.args.uri,
        )
        patch_service_base_size(self.args.service_config, OFFSET_PATH, None)
        manager.initialize_services(self.args.service_config, DELTA_TIME_USEC)

        server = make_protocol_server(
            pdu_manager=manager,
            service_name=SERVICE_NAME,
            srv="SystemControl",
            max_clients=1,
            ProtocolServerClass=ProtocolServerBlocking,
        )

        if not await server.start_service():
            logging.error("サービス開始に失敗しました")
            return

        async def system_control_handler(req: SystemControlRequest) -> SystemControlResponse:
            """システム制御サービスの実装。LauncherServiceを呼び出す。"""
            res = SystemControlResponse()
            if self.launcher_service is None:
                res.status_code = SystemControlStatusCode.FATAL
                res.message = "Launcher service not initialized"
                return res
            try:
                match req.opcode:
                    case SystemControlOpCode.ACTIVATE:
                        logging.info("RPC: ACTIVATE")
                        self.launcher_service.activate()
                    case SystemControlOpCode.START:
                        logging.info("RPC: START an asset simulation")
                        self.launcher_service.cmd('start')
                    case SystemControlOpCode.STOP:
                        logging.info("RPC: STOP an asset simulation")
                        self.launcher_service.cmd('stop')
                    case SystemControlOpCode.RESET:
                        logging.info("RPC: RESET an asset simulation")
                        self.launcher_service.cmd('reset')
                    case SystemControlOpCode.TERMINATE:
                        logging.info("RPC: TERMINATE all assets")
                        self.launcher_service.terminate()
                    case SystemControlOpCode.STATUS:
                        logging.info("RPC: STATUS")
                        status = self.launcher_service.status()
                        res.message = f"Status: {status}"
                    case _:
                        logging.warning(f"Unknown opcode: {req.opcode}")
                        res.status_code = SystemControlStatusCode.ERROR
                        res.message = f"Unknown opcode: {req.opcode}"
                        return res

                res.status_code = SystemControlStatusCode.OK
                if not res.message:
                    res.message = "OK"

            except Exception as e:
                logging.error(f"RPC handler error: {e}")
                res.status_code = SystemControlStatusCode.INTERNAL
                res.message = str(e)
            logging.info(f"RPC Response: {res.status_code}, {res.message}")
            return res

        logging.info(f"RPC Server started at {self.args.uri}")
        await server.serve(system_control_handler)
