import logging
import asyncio
import argparse
import sys
import os

# 親ディレクトリのモジュールをインポートするためにパスを追加
# server_rpc.pyがlauncher/hako_rpcにあるので、2つ上の階層をパスに追加
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.insert(0, project_root)

from hako_rpc.hako_rpc_server import HakoRpcServer
from hako_rpc.hako_drone_rpc_server import HakoDroneRpcServer

async def main() -> int:
    parser = argparse.ArgumentParser(description="Hakoniwa RPC Server")
    parser.add_argument("launch_file", help="Path to launcher JSON")
    parser.add_argument("--uri", default="ws://localhost:8080", help="WebSocketサーバのURI")
    parser.add_argument("--pdu-config", default="launcher/config/pdu_config.json")
    parser.add_argument("--service-config", default="launcher/config/service.json")
    parser.add_argument("--server-type", default="base", choices=['base', 'drone'], help="Type of RPC server to run")
    args = parser.parse_args()

    # Setup logging
    log_level = logging.DEBUG if os.environ.get('HAKO_PDU_DEBUG') == '1' else logging.INFO
    logging.basicConfig(
        level=log_level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        stream=sys.stdout
    )

    server = None
    try:
        if args.server_type == 'drone':
            logging.info("Starting HakoDroneRpcServer...")
            server = HakoDroneRpcServer(args)
        else:
            logging.info("Starting HakoRpcServer...")
            server = HakoRpcServer(args)

        await server.start()

    except Exception as e:
        logging.error(f"Server failed: {e}", file=sys.stderr)
        if server and server.launcher_service:
            server.launcher_service.terminate()
        return 1
    
    return 0

if __name__ == "__main__":
    asyncio.run(main())