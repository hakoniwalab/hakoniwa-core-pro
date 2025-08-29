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
from hakoniwa_pdu.rpc.auto_wire import make_protocol_servers
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
SYSTEM_CONTROL_SERVICE_NAME = "Service/SystemControl"

def _install_sigint(service: LauncherService):
    def _sigint_handler(signum, frame):
        print("[rpc_server] SIGINT received -> aborting...", file=sys.stderr)
        try:
            service.terminate()
        finally:
            sys.exit(1)
    signal.signal(signal.SIGINT, _sigint_handler)

class HakoBaseRpcServer:
    def __init__(self, args):
        self.args = args
        self.launcher_service = None
        self.services = []
        self.handlers = {}

    def add_service(self, name: str, srv_pkg: str, srv_type: str, handler, max_clients: int = 1):
        self.services.append({
            "service_name": name,
            "srv": srv_type,
            "pkg": srv_pkg,
            "max_clients": max_clients
        })
        self.handlers[name] = handler
        logging.info(f"Service '{name}' with handler '{handler.__name__}' added.")

    def _initialize_launcher(self):
        try:
            self.launcher_service = LauncherService(launch_path=self.args.launch_file)
            _install_sigint(self.launcher_service)
            logging.info("HakoRPCServer ready. assets:")
            for a in self.launcher_service.spec.assets:
                logging.info(f" - {a.name} (cwd={a.cwd}, cmd={a.command}, args={a.args})")
            
            # デフォルトのサービスを登録
            self.add_service(SYSTEM_CONTROL_SERVICE_NAME, "hakoniwa_pdu.pdu_msgs.hako_srv_msgs", "SystemControl", self._system_control_handler)
            return True
        except Exception as e:
            logging.error(f"Failed to load spec: {e}")
            return False

    async def _system_control_handler(self, req: SystemControlRequest) -> SystemControlResponse:
        res = SystemControlResponse()
        if self.launcher_service is None:
            res.status_code = SystemControlStatusCode.FATAL
            res.message = "Launcher service not initialized"
            return res
        try:
            match req.opcode:
                case SystemControlOpCode.ACTIVATE:
                    self.launcher_service.activate()
                case SystemControlOpCode.START:
                    self.launcher_service.cmd('start')
                case SystemControlOpCode.STOP:
                    self.launcher_service.cmd('stop')
                case SystemControlOpCode.RESET:
                    self.launcher_service.cmd('reset')
                case SystemControlOpCode.TERMINATE:
                    self.launcher_service.terminate()
                case SystemControlOpCode.STATUS:
                    res.message = f"Status: {self.launcher_service.status()}"
                case _:
                    res.status_code = SystemControlStatusCode.ERROR
                    res.message = f"Unknown opcode: {req.opcode}"
                    return res
            res.status_code = SystemControlStatusCode.OK
            if not res.message:
                res.message = "OK"
        except Exception as e:
            res.status_code = SystemControlStatusCode.INTERNAL
            res.message = str(e)
        return res

    async def start(self):
        if not self._initialize_launcher():
            return

        comm = WebSocketServerCommunicationService(version="v2")
        manager = RemotePduServiceServerManager(
            asset_name=ASSET_NAME,
            pdu_config_path=self.args.pdu_config,
            offset_path=self.args.offset_path,
            comm_service=comm,
            uri=self.args.uri,
        )
        patch_service_base_size(self.args.service_config, self.args.offset_path, None)
        manager.initialize_services(self.args.service_config, self.args.delta_time_usec)

        server = make_protocol_servers(
            pdu_manager=manager,
            services=self.services,
            ProtocolServerClass=ProtocolServerBlocking,
        )

        if not await server.start_services():
            logging.error("サービス開始に失敗しました")
            return

        logging.info(f"RPC Server started at {self.args.uri} with {len(self.services)} services.")
        await server.serve(self.handlers)