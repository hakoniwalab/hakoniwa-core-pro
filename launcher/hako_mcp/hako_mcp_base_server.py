import mcp.types as types
from mcp.server import Server
from pydantic import AnyUrl
import logging

# Imports from launch_client.py
from hakoniwa_pdu.impl.websocket_communication_service import WebSocketCommunicationService
from hakoniwa_pdu.rpc.remote.remote_pdu_service_client_manager import RemotePduServiceClientManager
from hakoniwa_pdu.rpc.auto_wire import make_protocol_client
from hakoniwa_pdu.rpc.protocol_client import ProtocolClientBlocking
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_SystemControlRequest import SystemControlRequest
from hakoniwa_pdu.rpc.codes import SystemControlOpCode

# Constants from launch_client.py (adjusted for server context)
ASSET_NAME = "HakoMcpServer"
CLIENT_NAME = "HakoMcpServer"
SERVICE_NAME = "Service/SystemControl"
OFFSET_PATH = "/usr/local/share/hakoniwa/offset" # This path might need adjustment
DELTA_TIME_USEC = 1_000_000

class HakoMcpBaseServer:
    def __init__(self, server_name, simulator_name="Simulator"):
        self.server = Server(server_name)
        self.simulator_name = simulator_name
        self.rpc_client = None
        self.rpc_uri = "ws://localhost:8080"
        self.pdu_config_path = "launcher/config/pdu_config.json"
        self.service_config_path = "launcher/config/service.json"

    async def initialize_rpc_client(self):
        logging.info("Initializing RPC client...")
        try:
            comm = WebSocketCommunicationService(version="v2")
            manager = RemotePduServiceClientManager(
                asset_name=ASSET_NAME,
                pdu_config_path=self.pdu_config_path,
                offset_path=OFFSET_PATH,
                comm_service=comm,
                uri=self.rpc_uri,
            )
            manager.initialize_services(self.service_config_path, DELTA_TIME_USEC)

            client = make_protocol_client(
                pdu_manager=manager,
                service_name=SERVICE_NAME,
                client_name=CLIENT_NAME,
                srv="SystemControl",
                ProtocolClientClass=ProtocolClientBlocking,
            )

            if not await client.start_service(self.rpc_uri):
                logging.error("Failed to start RPC communication service")
                return
            if not await client.register():
                logging.error("Failed to register RPC client")
                return
            
            self.rpc_client = client
            logging.info("RPC client initialized and registered successfully.")
        except Exception as e:
            logging.error(f"Failed to initialize RPC client: {e}")
            self.rpc_client = None

    async def _send_rpc_command(self, opcode):
        if not self.rpc_client:
            error_msg = "RPC client is not initialized."
            logging.error(error_msg)
            return error_msg
        
        try:
            req = SystemControlRequest()
            req.opcode = opcode
            res = await self.rpc_client.call(req, timeout_msec=1000)
            if res is None:
                error_msg = f"RPC call failed for opcode: {opcode}"
                logging.error(error_msg)
                return error_msg
            
            logging.info(f"RPC Response for opcode {opcode}: {res.message}")
            return f"RPC Response: {res.message}"
        except Exception as e:
            error_msg = f"An error occurred during RPC call for opcode {opcode}: {e}"
            logging.error(error_msg)
            return error_msg

    async def hakoniwa_simulator_start(self) -> str:
        if not await self._send_rpc_command(SystemControlOpCode.ACTIVATE):
            return "Failed to activate simulator."
        if not await self._send_rpc_command(SystemControlOpCode.START):
            return "Failed to start simulator."
        return "Simulator started successfully."

    async def hakoniwa_simulator_stop(self) -> str:
        if not await self._send_rpc_command(SystemControlOpCode.TERMINATE):
            return "Failed to terminate simulator."
        return "Simulator terminated successfully."

    async def list_tools(self) -> list[types.Tool]:
        return [
            types.Tool(
                name="hakoniwa_simulator_start",
                description=f"Starts the Hakoniwa {self.simulator_name}",
                inputSchema={"type": "object", "properties": {}, "required": []}
            ),
            types.Tool(
                name="hakoniwa_simulator_stop",
                description=f"Stops the Hakoniwa {self.simulator_name}",
                inputSchema={"type": "object", "properties": {}, "required": []}
            ),
        ]

    async def call_tool(self, name: str, arguments: dict | None) -> list[types.TextContent]:
        if name == "hakoniwa_simulator_start":
            result = await self.hakoniwa_simulator_start()
            return [types.TextContent(type="text", text=result)]
        elif name == "hakoniwa_simulator_stop":
            result = await self.hakoniwa_simulator_stop()
            return [types.TextContent(type="text", text=result)]
        else:
            raise ValueError(f"Unknown tool: {name}")

    async def list_resources(self) -> list[types.Resource]:
        return []

    async def read_resource(self, uri: AnyUrl) -> str:
        raise NotImplementedError("Resource reading is not supported.")