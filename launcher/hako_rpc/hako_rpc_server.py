from __future__ import annotations
import logging
import asyncio
import argparse
import sys
import signal
import os

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

# 親ディレクトリからLauncherServiceをインポート
from hako_launch.hako_launcher import LauncherService

# 定数
ASSET_NAME = "HakoRpcBaseServer"
SERVICE_NAME = "Service/SystemControl"
OFFSET_PATH = "/Users/tmori/project/oss/hakoniwa-pdu-python/tests/config/offset"
DELTA_TIME_USEC = 1_000_000

class HakoRpcServer:
    def __init__(self, launcher_service: LauncherService, args):
        self.launcher_service = launcher_service
        self.args = args

    async def start(self):
        """RPCサーバーを起動し、リクエストを待ち受けます。"""
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

def _install_sigint(service: LauncherService):
    def _sigint_handler(signum, frame):
        print("[rpc_server] SIGINT received → aborting...")
        try:
            service.terminate()
        finally:
            sys.exit(1)
    signal.signal(signal.SIGINT, _sigint_handler)

async def main() -> int:
    parser = argparse.ArgumentParser(description="Hakoniwa RPC Server")
    parser.add_argument("launch_file", help="Path to launcher JSON")
    parser.add_argument("--uri", default="ws://localhost:8080", help="WebSocketサーバのURI")
    parser.add_argument("--pdu-config", default="launcher/config/pdu_config.json")
    parser.add_argument("--service-config", default="launcher/config/service.json")
    args = parser.parse_args()

    # Setup logging
    if os.environ.get('HAKO_PDU_DEBUG') == '1':
        log_level = logging.DEBUG
    else:
        log_level = logging.INFO

    logging.basicConfig(
        level=log_level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        stream=sys.stdout
    )

    try:
        service = LauncherService(launch_path=args.launch_file)
    except Exception as e:
        print(f"[rpc_server] Failed to load spec: {e}", file=sys.stderr)
        return 1

    _install_sigint(service)

    print("[INFO] HakoRPCServer ready. assets:")
    for a in service.spec.assets:
        print(f" - {a.name} (cwd={a.cwd}, cmd={a.command}, args={a.args})")

    try:
        server = HakoRpcServer(service, args)
        await server.start()
    except Exception as e:
        print(f"[rpc_server] Server failed: {e}", file=sys.stderr)
        service.terminate()
        return 1
    
    return 0

if __name__ == "__main__":
    asyncio.run(main())
