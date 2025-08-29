import logging
import asyncio
import argparse
import sys
import os

from launcher.hako_rpc.hako_base_rpc_server import HakoBaseRpcServer
from hako_rpc.hako_drone_rpc_server import HakoDroneRpcServer

async def main() -> int:
    parser = argparse.ArgumentParser(description="Hakoniwa RPC Server")
    parser.add_argument("launch_file", help="Path to launcher JSON")
    parser.add_argument("--uri", default="ws://localhost:8080", help="WebSocketサーバのURI")
    parser.add_argument("--pdu-config", default="launcher/config/pdu_config.json")
    parser.add_argument("--service-config", default="launcher/config/service.json")
    parser.add_argument("--offset-path", default="/usr/local/share/hakoniwa/offset")
    parser.add_argument("--server-type", default="base", choices=['base', 'drone'], help="Type of RPC server to run")
    parser.add_argument("--delta-time-usec", default=1000000, type=int, help="Delta time in microseconds")
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
            logging.info("Starting HakoBaseRpcServer...")
            server = HakoBaseRpcServer(args)

        await server.start()

    except Exception as e:
        logging.error(f"Server failed: {e}", file=sys.stderr)
        if server and server.launcher_service:
            server.launcher_service.terminate()
        return 1
    
    return 0

if __name__ == "__main__":
    asyncio.run(main())