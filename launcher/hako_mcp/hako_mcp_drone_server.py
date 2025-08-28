from .hako_mcp_base_server import HakoMcpBaseServer
import mcp.types as types
import json
import logging

try:
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneSetReadyRequest import DroneSetReadyRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneSetReadyResponse import DroneSetReadyResponse
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneTakeOffRequest import DroneTakeOffRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneTakeOffResponse import DroneTakeOffResponse
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneGoToRequest import DroneGoToRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneGoToResponse import DroneGoToResponse
    
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneGetStateRequest import DroneGetStateRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneGetStateResponse import DroneGetStateResponse
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneLandRequest import DroneLandRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_DroneLandResponse import DroneLandResponse
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_CameraCaptureImageRequest import CameraCaptureImageRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_CameraCaptureImageResponse import CameraCaptureImageResponse
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_CameraSetTiltRequest import CameraSetTiltRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_CameraSetTiltResponse import CameraSetTiltResponse

    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_LiDARScanRequest import LiDARScanRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_LiDARScanResponse import LiDARScanResponse
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_MagnetGrabRequest import MagnetGrabRequest
    from hakoniwa_pdu.pdu_msgs.drone_srv_msgs.pdu_pytype_MagnetGrabResponse import MagnetGrabResponse
except ImportError:
    logging.error("PDU types not found, using dummy classes.")

class HakoMcpDroneServer(HakoMcpBaseServer):
    def __init__(self, server_name="hakoniwa_drone"):
        super().__init__(server_name, simulator_name="Drone")
        self.drone_service_config_path = "launcher/config/drone_service.json"
        self._register_drone_rpc_services()

    def _register_drone_rpc_services(self):
        try:
            with open(self.drone_service_config_path, 'r') as f:
                config = json.load(f)
            
            for service_def in config["services"]:
                service_name = service_def["name"]
                if service_name.startswith("DroneService/"):
                    srv_type = service_def["type"].split('/')[-1]
                    self.add_rpc_service(service_name, srv_type)

        except FileNotFoundError:
            logging.error(f"Drone service config not found: {self.drone_service_config_path}")
        except Exception as e:
            logging.error(f"Error registering drone RPC services: {e}")

    async def list_tools(self) -> list[types.Tool]:
        base_tools = await super().list_tools()
        drone_tools = [
            types.Tool(
                name="drone_takeoff",
                description="Executes a takeoff command on the drone simulator.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "drone_name": {"type": "string", "description": "The name of the drone to command."},
                        "height": {"type": "number", "description": "The target height for takeoff in meters."}
                    },
                    "required": ["drone_name", "height"]
                }
            ),
            types.Tool(
                name="drone_land",
                description="Executes a land command on the drone simulator.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "drone_name": {"type": "string", "description": "The name of the drone to command."}
                    },
                    "required": ["drone_name"]
                }
            ),
            # TODO: Add other drone tools (GoTo, GetState, etc.) here
        ]
        return base_tools + drone_tools

    async def call_tool(self, name: str, arguments: dict | None) -> list[types.TextContent]:
        try:
            return await super().call_tool(name, arguments)
        except ValueError:
            if arguments is None:
                raise ValueError(f"Arguments required for tool: {name}")

            if name == "drone_takeoff":
                req = DroneTakeOffRequest()
                req.body.drone_name = arguments["drone_name"]
                req.body.alt_m = arguments["height"]
                result = await self._send_rpc_command("DroneService/DroneTakeoff", req)
                return [types.TextContent(type="text", text=result)]
            
            elif name == "drone_land":
                req = DroneLandRequest()
                req.body.drone_name = arguments["drone_name"]
                result = await self._send_rpc_command("DroneService/DroneLand", req)
                return [types.TextContent(type="text", text=result)]

            # TODO: Add other drone tool handlers here
            else:
                raise ValueError(f"Unknown tool: {name}")