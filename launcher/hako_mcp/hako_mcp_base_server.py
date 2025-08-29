import mcp.types as types
from mcp.server import Server
from pydantic import AnyUrl
import logging

from hakoniwa_pdu.impl.websocket_communication_service import WebSocketCommunicationService
from hakoniwa_pdu.rpc.remote.remote_pdu_service_client_manager import RemotePduServiceClientManager
from hakoniwa_pdu.rpc.auto_wire import make_protocol_clients
from hakoniwa_pdu.rpc.protocol_client import ProtocolClientBlocking
from hakoniwa_pdu.pdu_msgs.hako_srv_msgs.pdu_pytype_SystemControlRequest import SystemControlRequest
from hakoniwa_pdu.rpc.codes import SystemControlOpCode

ASSET_NAME = "HakoMcpServer"
CLIENT_NAME_DEFAULT = "HakoMcpServerClient"
SYSTEM_CONTROL_SERVICE_NAME = "Service/SystemControl"
OFFSET_PATH = "/usr/local/share/hakoniwa/offset"
DELTA_TIME_USEC = 1_000_000

class HakoMcpBaseServer:
    def __init__(self, server_name, pdu_config_path: str, service_config_path: str, simulator_name="Simulator"):
        self.server = Server(server_name)
        self.simulator_name = simulator_name
        self.rpc_clients = {}
        self.rpc_service_specs = []
        self.rpc_uri = "ws://localhost:8080"
        self.pdu_config_path = pdu_config_path
        self.service_config_path = service_config_path

    def add_rpc_service(self, service_name: str, srv_pkg: str, srv_type: str, client_name: str = CLIENT_NAME_DEFAULT):
        self.rpc_service_specs.append({
            "service_name": service_name,
            "pkg": srv_pkg,
            "srv": srv_type,
            "client_name": client_name
        })
        logging.info(f"RPC service spec added: {service_name}")

    async def initialize_rpc_clients(self):
        logging.info("Initializing RPC clients...")
        self.add_rpc_service(SYSTEM_CONTROL_SERVICE_NAME, "hakoniwa_pdu.pdu_msgs.hako_srv_msgs", "SystemControl")
        
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

            self.rpc_clients = make_protocol_clients(
                pdu_manager=manager,
                services=self.rpc_service_specs,
                ProtocolClientClass=ProtocolClientBlocking,
            )

            if not await manager.start_service(self.rpc_uri):
                raise ConnectionError(f"Failed to start RPC service for {name}")
            for name, client in self.rpc_clients.items():
                if not await client.register():
                    raise ConnectionError(f"Failed to register RPC client for {name}")
            
            logging.info(f"RPC clients initialized and registered successfully for {len(self.rpc_clients)} services.")
        except Exception as e:
            logging.error(f"Failed to initialize RPC clients: {e}")
            self.rpc_clients = {}

    async def _send_rpc_command(self, service_name: str, req_pdu):
        if service_name not in self.rpc_clients:
            logging.error(f"RPC client for service '{service_name}' is not initialized.")
            return None
        
        client = self.rpc_clients[service_name]
        try:
            res = await client.call(req_pdu, timeout_msec=-1)
            if res is None:
                logging.error(f"RPC call failed for service: {service_name}")
                return None
            
            logging.info(f"RPC Response from {service_name}: {res.message}")
            return res
        except Exception as e:
            logging.error(f"An error occurred during RPC call to {service_name}: {e}")
            return None

    async def hakoniwa_simulator_activate(self) -> str:
        req = SystemControlRequest()
        req.opcode = SystemControlOpCode.ACTIVATE
        res = await self._send_rpc_command(SYSTEM_CONTROL_SERVICE_NAME, req)
        return res.message if res else "RPC failed"
    
    async def hakoniwa_simulator_start(self) -> str:
        req = SystemControlRequest()
        req.opcode = SystemControlOpCode.START
        res = await self._send_rpc_command(SYSTEM_CONTROL_SERVICE_NAME, req)
        return res.message if res else "RPC failed"

    async def hakoniwa_simulator_terminate(self) -> str:
        req = SystemControlRequest()
        req.opcode = SystemControlOpCode.TERMINATE
        res = await self._send_rpc_command(SYSTEM_CONTROL_SERVICE_NAME, req)
        return res.message if res else "RPC failed"

    async def list_tools(self) -> list[types.Tool]:
        return [
            types.Tool(
                name="hakoniwa_simulator_activate",
                description=f"Activate the Hakoniwa {self.simulator_name}",
                inputSchema={"type": "object", "properties": {}, "required": []}
            ),
            types.Tool(
                name="hakoniwa_simulator_start",
                description=f"Start the Hakoniwa {self.simulator_name}",
                inputSchema={"type": "object", "properties": {}, "required": []}
            ),
            types.Tool(
                name="hakoniwa_simulator_terminate",
                description=f"Terminate the Hakoniwa {self.simulator_name}",
                inputSchema={"type": "object", "properties": {}, "required": []}
            ),
        ]

    async def call_tool(self, name: str, arguments: dict | None) -> list[types.TextContent]:
        logging.info(f"Calling tool: {name} with arguments: {arguments}")
        if name == "hakoniwa_simulator_activate":
            result = await self.hakoniwa_simulator_activate()
            return [types.TextContent(type="text", text=result)]
        elif name == "hakoniwa_simulator_start":
            result = await self.hakoniwa_simulator_start()
            return [types.TextContent(type="text", text=result)]
        elif name == "hakoniwa_simulator_terminate":
            result = await self.hakoniwa_simulator_terminate()
            return [types.TextContent(type="text", text=result)]
        else:
            raise ValueError(f"Unknown tool: {name}")

    async def list_resources(self) -> list[types.Resource]:
        return []

    async def read_resource(self, uri: AnyUrl) -> str:
        raise NotImplementedError("Resource reading is not supported.")