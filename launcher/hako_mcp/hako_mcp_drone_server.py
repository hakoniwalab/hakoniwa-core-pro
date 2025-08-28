from .hako_mcp_base_server import HakoMcpBaseServer
import mcp.types as types
import json
import logging
import base64

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
                name="drone_set_ready",
                description="Prepare the drone for mission.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]}
            ),
            types.Tool(
                name="drone_takeoff",
                description="Executes a takeoff command.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "height": {"type": "number", "description": "Target height in meters."}}, "required": ["drone_name", "height"]}
            ),
            types.Tool(
                name="drone_land",
                description="Executes a land command.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]}
            ),
            types.Tool(
                name="drone_get_state",
                description="Get the drone's current state.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]},
                outputSchema={"type": "object", "properties": {"ok": {"type": "boolean"}, "is_ready": {"type": "boolean"}, "current_pose": {"type": "object"}, "mode": {"type": "string"}, "message": {"type": "string"}}}
            ),
            types.Tool(
                name="drone_go_to",
                description="Move drone to a specified position in the ROS coordinate system.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "drone_name": {"type": "string"},
                        "x": {"type": "number", "description": "Target X in meters."},
                        "y": {"type": "number", "description": "Target Y in meters."},
                        "z": {"type": "number", "description": "Target Z in meters."},
                        "speed": {"type": "number", "description": "Speed in meters/second."},
                        "yaw": {"type": "number", "description": "Yaw angle in degrees."},
                        "tolerance": {"type": "number", "description": "Position tolerance in meters."},
                        "timeout": {"type": "number", "description": "Timeout in seconds."}
                    },
                    "required": ["drone_name", "x", "y", "z", "speed", "yaw", "tolerance", "timeout"]
                }
            ),
            types.Tool(
                name="camera_capture_image",
                description="Capture an image from the drone's camera.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "image_type": {"type": "string", "description": "e.g., 'png' or 'jpeg'."}}, "required": ["drone_name", "image_type"]},
                outputSchema={
                    "type": "object",
                    "properties": {
                        "ok": {"type": "boolean"},
                        "message": {"type": "string"},
                        "image": {"type": "object", "properties": {"format": {"type": "string"}, "data": {"type": "string", "contentEncoding": "base64"}}}
                    }
                }
            ),
            types.Tool(
                name="camera_set_tilt",
                description="Set the tilt angle of the drone's camera.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "angle": {"type": "number", "description": "Tilt angle in degrees."}}, "required": ["drone_name", "angle"]}
            ),
            types.Tool(
                name="lidar_scan",
                description="Perform a LiDAR scan.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]},
                outputSchema={"type": "object", "properties": {"ok": {"type": "boolean"}, "message": {"type": "string"}, "point_cloud": {"type": "object"}, "lidar_pose": {"type": "object"}}}
            ),
            types.Tool(
                name="magnet_grab",
                description="Control the drone's magnet.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "grab": {"type": "boolean"}, "timeout": {"type": "number"}}, "required": ["drone_name", "grab", "timeout"]},
                outputSchema={"type": "object", "properties": {"ok": {"type": "boolean"}, "message": {"type": "string"}, "magnet_on": {"type": "boolean"}, "contact_on": {"type": "boolean"}}}
            )
        ]
        return base_tools + drone_tools

    async def call_tool(self, name: str, arguments: dict | None) -> list[types.TextContent]:
        try:
            return await super().call_tool(name, arguments)
        except ValueError:
            if arguments is None:
                raise ValueError(f"Arguments required for tool: {name}")
            
            drone_name = arguments.get("drone_name")
            if not drone_name:
                raise ValueError("'drone_name' is a required argument.")

            result_pdu = None
            if name == "drone_set_ready":
                req = DroneSetReadyRequest(); req.body.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/DroneSetReady", req)
            elif name == "drone_takeoff":
                req = DroneTakeOffRequest(); req.body.drone_name = drone_name; req.body.alt_m = arguments["height"]
                result_pdu = await self._send_rpc_command("DroneService/DroneTakeOff", req)
            elif name == "drone_land":
                req = DroneLandRequest(); req.body.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/DroneLand", req)
            elif name == "drone_get_state":
                req = DroneGetStateRequest(); req.body.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/DroneGetState", req)
            elif name == "drone_go_to":
                req = DroneGoToRequest(); req.body.drone_name = drone_name; req.body.target_pose = Vector3(); req.body.target_pose.x = arguments["x"]; req.body.target_pose.y = arguments["y"]; req.body.target_pose.z = arguments["z"]; req.body.speed_m_s = arguments["speed"]; req.body.yaw_deg = arguments["yaw"]; req.body.tolerance_m = arguments["tolerance"]; req.body.timeout_sec = arguments["timeout"]
                result_pdu = await self._send_rpc_command("DroneService/DroneGoTo", req)
            elif name == "camera_capture_image":
                req = CameraCaptureImageRequest(); req.body.drone_name = drone_name; req.body.image_type = arguments["image_type"]
                result_pdu = await self._send_rpc_command("DroneService/CameraCaptureImage", req)
                if result_pdu and result_pdu.body.ok:
                    res_dict = result_pdu.body.to_dict()
                    if 'image' in res_dict and 'data' in res_dict['image']:
                        byte_data = bytes(res_dict['image']['data'])
                        res_dict['image']['data'] = base64.b64encode(byte_data).decode('utf-8')
                    return [types.TextContent(type="text", text=json.dumps(res_dict, indent=2))]
            elif name == "camera_set_tilt":
                req = CameraSetTiltRequest(); req.body.drone_name = drone_name; req.body.tilt_angle_deg = arguments["angle"]
                result_pdu = await self._send_rpc_command("DroneService/CameraSetTilt", req)
            elif name == "lidar_scan":
                req = LiDARScanRequest(); req.body.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/LiDARScan", req)
            elif name == "magnet_grab":
                req = MagnetGrabRequest(); req.body.drone_name = drone_name; req.body.grab_on = arguments["grab"]; req.body.timeout_sec = arguments["timeout"]
                result_pdu = await self._send_rpc_command("DroneService/MagnetGrab", req)
            else:
                raise ValueError(f"Unknown tool: {name}")
            
            if result_pdu:
                return [types.TextContent(type="text", text=result_pdu.body.to_json())]
            else:
                return [types.TextContent(type="text", text=json.dumps({"ok": False, "message": "RPC call failed."}))]
