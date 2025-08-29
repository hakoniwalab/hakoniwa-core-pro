from .hako_mcp_base_server import HakoMcpBaseServer
import mcp.types as types
#from mcp.types import CallToolResult
import json
import logging
import os
from examples.drone_sample.libs.hakosim_lidar import LidarData, LiDARFilter

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
    from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_pytype_Vector3 import Vector3
except ImportError:
    logging.error("PDU types not found, using dummy classes.")
drone_get_state_output_schema = {
    "type": "object",
    "additionalProperties": False,
    "properties": {
        "ok": {"type": "boolean"},
        "is_ready": {"type": "boolean"},
        "current_pose": {
            "type": "object",
            "additionalProperties": False,
            "properties": {
                "position": {
                    "type": "object",
                    "additionalProperties": False,
                    "properties": {
                        "x": {"type": "number"},
                        "y": {"type": "number"},
                        "z": {"type": "number"}
                    },
                    "required": ["x", "y", "z"]
                },
                "orientation": {
                    "type": "object",
                    "additionalProperties": False,
                    "properties": {
                        "x": {"type": "number"},
                        "y": {"type": "number"},
                        "z": {"type": "number"},
                        "w": {"type": "number"}
                    },
                    "required": ["x", "y", "z", "w"]
                }
            },
            "required": ["position", "orientation"]
        },
        "battery_status": {
            "type": "object",
            "additionalProperties": False,
            "properties": {
                "full_voltage": {"type": "number"},
                "curr_voltage": {"type": "number"},
                "curr_temp": {"type": "number"},
                "status": {"type": "number"},
                "cycles": {"type": "number"}
            },
            "required": ["full_voltage", "curr_voltage", "curr_temp", "status", "cycles"]
        },
        "mode": {"type": "string"},
        "message": {"type": "string"}
    },
    "required": ["ok", "is_ready", "current_pose", "battery_status", "mode", "message"]
}

class HakoMcpDroneServer(HakoMcpBaseServer):
    def __init__(self, pdu_config_path: str, service_config_path: str, server_name="hakoniwa_drone"):
        super().__init__(server_name, pdu_config_path, service_config_path, simulator_name="Drone")
        self._register_drone_rpc_services()

    def _register_drone_rpc_services(self):
        try:
            with open(self.service_config_path, 'r') as f:
                config = json.load(f)
            for service_def in config["services"]:
                service_name = service_def["name"]
                if service_name.startswith("DroneService/"):
                    srv_type = service_def["type"].split('/')[-1]
                    self.add_rpc_service(service_name, "hakoniwa_pdu.pdu_msgs.drone_srv_msgs", srv_type)
        except FileNotFoundError:
            logging.error(f"Service config not found: {self.service_config_path}")
        except Exception as e:
            logging.error(f"Error registering drone RPC services: {e}")

    async def list_tools(self) -> list[types.Tool]:
        global drone_get_state_output_schema
        base_tools = await super().list_tools()
        drone_tools = [
            types.Tool(
                name="drone_set_ready",
                description="Prepare the drone for mission. The default value for drone_name is 'Drone'.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]}
            ),
            types.Tool(
                name="drone_takeoff",
                description="Executes a takeoff command. The default value for drone_name is 'Drone'.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "height": {"type": "number", "description": "Target height in meters."}}, "required": [ "height"]}
            ),
            types.Tool(
                name="drone_land",
                description="Executes a land command. The default value for drone_name is 'Drone'.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]}
            ),
            types.Tool(
                name="drone_get_state",
                description="Get the drone's current state. The default value for drone_name is 'Drone'.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}}, "required": ["drone_name"]},
                #outputSchema=drone_get_state_output_schema
            ),
            types.Tool(
                name="drone_go_to",
                description="Move drone to a specified position in the ROS coordinate system. The default value for drone_name is 'Drone'.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "drone_name": {"type": "string"},
                        "x": {"type": "number", "description": "Target X in meters."},
                        "y": {"type": "number", "description": "Target Y in meters."},
                        "z": {"type": "number", "description": "Target Z in meters."},
                        "speed": {"type": "number", "description": "Speed in meters/second. Default is 1.0."},
                        "yaw": {"type": "number", "description": "Yaw angle in degrees. Default is 0.0."},
                        "tolerance": {"type": "number", "description": "Position tolerance in meters. Default is 0.5."},
                        "timeout": {"type": "number", "description": "Unsupported. Please always use -1."}
                    },
                    "required": ["drone_name", "x", "y", "z"]
                }
            ),
            types.Tool(
                name="camera_capture_image",
                description=(
                    "Capture an image from the drone's camera and save it to the given filepath. "
                    "The 'filepath' must be specified by the caller and should end with '.png'. "
                    "The default value for 'drone_name' is 'Drone'. "
                    "The default value for 'image_type' is 'png'."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "drone_name": {"type": "string"},
                        "filepath": {
                            "type": "string",
                            "description": "Destination file path (must end with '.png')."
                        },
                        "image_type": {
                            "type": "string",
                            "enum": ["png"],
                            "description": "Image format (currently only 'png' is supported)."
                        }
                    },
                    "required": ["filepath"]
                }
                #outputSchema={
                #    "type": "object",
                #    "properties": {
                #        "ok": {"type": "boolean"},
                #        "message": {"type": "string"},
                #        "image": {"type": "object", "properties": {"format": {"type": "string"}, "data": {"type": "string", "contentEncoding": "base64"}}}
                #    }
                #}
            ),
            types.Tool(
                name="camera_set_tilt",
                description="Set the tilt angle of the drone's camera. The default value for drone_name is 'Drone'.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "angle": {"type": "number", "description": "Tilt angle in degrees."}}, "required": ["drone_name", "angle"]}
            ),
            types.Tool(
                name="lidar_scan",
                description=(
                    "Perform a LiDAR scan and return lidar_pose and filtered point cloud data. "
                    "The default value for drone_name is 'Drone'. "
                    "Filtering parameters can be specified: "
                    "x_size, y_size (grid cell size in meters, default 0.4), "
                    "min_r, max_r (min/max distance from LiDAR in meters, default 0.3/10.0), "
                    "z_band (min/max Z-coordinate in meters, default -0.2/2.5), "
                    "top_k (Number of closest candidates to return, default 10), "
                    "with_stats (boolean to include detailed statistics, default false)."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "drone_name": {"type": "string"},
                        "x_size": {"type": "number", "default": 0.4, "description": "Grid cell X size in meters."},
                        "y_size": {"type": "number", "default": 0.4, "description": "Grid cell Y size in meters."},
                        "min_r": {"type": "number", "default": 0.3, "description": "Minimum distance from LiDAR in meters."},
                        "max_r": {"type": "number", "default": 10.0, "description": "Maximum distance from LiDAR in meters."},
                        "z_band": {
                            "type": "array",
                            "items": {"type": "number"},
                            "minItems": 2,
                            "maxItems": 2,
                            "default": [-0.2, 2.5],
                            "description": "Min and max Z-coordinate in meters [min_z, max_z]."
                        },
                        "top_k": {"type": "integer", "default": 10, "description": "Number of closest points to return per cell."},
                        "with_stats": {"type": "boolean", "default": False, "description": "Include detailed statistics for each point."}
                    },
                    "required": ["drone_name"]
                },
                #outputSchema={"type": "object", "properties": {"ok": {"type": "boolean"}, "message": {"type": "string"}, "point_cloud": {"type": "object"}, "lidar_pose": {"type": "object"}}}
            ),
            types.Tool(
                name="magnet_grab",
                description="Control the drone's magnet. The default value for drone_name is 'Drone'.",
                inputSchema={"type": "object", "properties": {"drone_name": {"type": "string"}, "grab": {"type": "boolean"}, "timeout": {"type": "number", "description": "Unsupported. Please always use -1."}}, "required": ["drone_name", "grab"]},
                #outputSchema={"type": "object", "properties": {"ok": {"type": "boolean"}, "message": {"type": "string"}, "magnet_on": {"type": "boolean"}, "contact_on": {"type": "boolean"}}}
            )
        ]
        return base_tools + drone_tools

    async def call_tool(self, name: str, arguments: dict | None) -> list[types.TextContent]:
        try:
            return await super().call_tool(name, arguments)
        except ValueError:
            if arguments is None:
                arguments = {}
            
            drone_name = arguments.get("drone_name", "Drone")

            result_pdu = None
            if name == "drone_set_ready":
                req = DroneSetReadyRequest(); req.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/DroneSetReady", req)
            elif name == "drone_takeoff":
                req = DroneTakeOffRequest(); req.drone_name = drone_name; req.alt_m = arguments["height"]
                result_pdu = await self._send_rpc_command("DroneService/DroneTakeOff", req)
            elif name == "drone_land":
                req = DroneLandRequest(); req.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/DroneLand", req)
            elif name == "drone_get_state":
                req = DroneGetStateRequest(); req.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/DroneGetState", req)
            elif name == "drone_go_to":
                req = DroneGoToRequest(); req.drone_name = drone_name; req.target_pose = Vector3(); req.target_pose.x = arguments["x"]; req.target_pose.y = arguments["y"]; req.target_pose.z = arguments["z"]; req.speed_m_s = arguments.get("speed", 1.0); req.yaw_deg = arguments.get("yaw", 0.0); req.tolerance_m = arguments.get("tolerance", 0.5); req.timeout_sec = -1
                result_pdu = await self._send_rpc_command("DroneService/DroneGoTo", req)
            elif name == "camera_capture_image":
                req = CameraCaptureImageRequest(); req.drone_name = drone_name; req.image_type = arguments.get("image_type", "png")
                filepath: str | None = arguments.get("filepath")
                if not filepath:
                    raise ValueError("`filepath` is required.")
                if not filepath.lower().endswith(".png"):
                    raise ValueError("`filepath` must end with '.png'.")

                os.makedirs(os.path.dirname(filepath) or ".", exist_ok=True)

                result_pdu = await self._send_rpc_command("DroneService/CameraCaptureImage", req)
                if result_pdu and result_pdu.ok:
                    res_dict = result_pdu.to_dict()
                    if 'data' in res_dict and filepath is not None:
                        byte_data = bytes(res_dict['data'])
                        with open(filepath, 'wb') as img_file:
                            img_file.write(byte_data)
                        res_dict['data'] = None
                    return [types.TextContent(type="text", text=json.dumps(res_dict, indent=2))]
            elif name == "camera_set_tilt":
                req = CameraSetTiltRequest(); req.drone_name = drone_name; req.tilt_angle_deg = arguments["angle"]
                result_pdu = await self._send_rpc_command("DroneService/CameraSetTilt", req)
            elif name == "lidar_scan":
                req = LiDARScanRequest(); req.drone_name = drone_name
                result_pdu = await self._send_rpc_command("DroneService/LiDARScan", req)
                if result_pdu and result_pdu.ok:
                    # PointCloud2からフラットなXYZリストを抽出
                    point_cloud_bytes = result_pdu.point_cloud.data
                    row_step = result_pdu.point_cloud.row_step
                    height = result_pdu.point_cloud.height
                    total_data_bytes = height * row_step
                    flat_xyz_list = LidarData.extract_xyz_from_point_cloud(point_cloud_bytes, total_data_bytes)

                    # LiDARの姿勢を(x, y, z)タプルに変換
                    lidar_pose_tuple = (
                        result_pdu.lidar_pose.position.x,
                        result_pdu.lidar_pose.position.y,
                        result_pdu.lidar_pose.position.z
                    )

                    # LidarDataインスタンスを作成
                    # time_stampはheaderから取得
                    time_stamp = result_pdu.point_cloud.header.stamp.sec + result_pdu.point_cloud.header.stamp.nanosec / 1e9
                    lidar_data = LidarData(
                        point_cloud=flat_xyz_list,
                        time_stamp=time_stamp,
                        pose=lidar_pose_tuple,
                        data_frame='VehicleInertialFrame'
                    )

                    # LiDARFilterを適用
                    lidar_filter = LiDARFilter(lidar_data)

                    # フィルタリングパラメータをargumentsから取得、デフォルト値を考慮
                    x_size = arguments.get("x_size", 0.4)
                    y_size = arguments.get("y_size", 0.4)
                    min_r = arguments.get("min_r", 0.3)
                    max_r = arguments.get("max_r", 10.0)
                    z_band_list = arguments.get("z_band", [-0.2, 2.5])
                    if (not isinstance(z_band_list, (list, tuple))) or len(z_band_list) != 2:
                        return [types.TextContent(type="text", text=json.dumps(
                            {"ok": False, "message": "z_band must be [min_z, max_z]"}
                        ))]
                    z0, z1 = float(z_band_list[0]), float(z_band_list[1])
                    if z0 > z1:
                        z0, z1 = z1, z0
                    z_band = (z0, z1)                    
                    top_k = arguments.get("top_k", 10)
                    with_stats = arguments.get("with_stats", False)

                    filtered_points = lidar_filter.filter(
                        x_size=x_size,
                        y_size=y_size,
                        min_r=min_r,
                        max_r=max_r,
                        z_band=z_band,
                        top_k=top_k,
                        with_stats=with_stats
                    )

                    # フィルタリング結果をJSONで返す
                    return [types.TextContent(type="text", text=json.dumps({"ok": True, "lidar_pose": result_pdu.lidar_pose.to_dict(), "filtered_points": filtered_points}))]
                else:
                    return [types.TextContent(type="text", text=json.dumps({"ok": False, "message": "LiDAR scan failed."}))]
            elif name == "magnet_grab":
                req = MagnetGrabRequest(); req.drone_name = drone_name; req.grab_on = arguments["grab"]; req.timeout_sec = -1
                result_pdu = await self._send_rpc_command("DroneService/MagnetGrab", req)
            else:
                raise ValueError(f"Unknown tool: {name}")
            
            if result_pdu:
                return [types.TextContent(type="text", text=result_pdu.to_json())]
            else:
                return [types.TextContent(type="text", text=json.dumps({"ok": False, "message": "RPC call failed."}))]
