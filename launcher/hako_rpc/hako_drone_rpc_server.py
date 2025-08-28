from __future__ import annotations
import logging
import json

from .hako_rpc_server import HakoRpcServer
from hako_drone.hako_drone_service import HakoDroneService
from hakoniwa_pdu.rpc.codes import SystemControlStatusCode

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



class HakoDroneRpcServer(HakoRpcServer):
    def __init__(self, args):
        super().__init__(args)
        self.drone_service = None
        self.drone_service_config_path = "launcher/config/drone_service.json"
        logging.info("HakoDroneRpcServer initialized")

    def _initialize_launcher(self):
        if not super()._initialize_launcher():
            return False
        
        if self.launcher_service:
            self.drone_service = HakoDroneService(self.launcher_service, self.args.launch_file)
            self._register_drone_services()
            return True
        return False

    def _register_drone_services(self):
        try:
            with open(self.drone_service_config_path, 'r') as f:
                config = json.load(f)
            
            handler_map = {
                "DroneService/DroneSetReady": self._set_ready_handler,
                "DroneService/DroneTakeoff": self._takeoff_handler,
                "DroneService/DroneGoTo": self._go_to_handler,
                "DroneService/DroneGetState": self._get_state_handler,
                "DroneService/DroneLand": self._land_handler,
                "DroneService/CameraCaptureImage": self._capture_image_handler,
                "DroneService/CameraSetTilt": self._set_tilt_handler,
                "DroneService/LiDARScan": self._scan_lidar_handler,
                "DroneService/MagnetGrab": self._grab_magnet_handler,
            }

            for service_def in config["services"]:
                service_name = service_def["name"]
                if service_name in handler_map:
                    srv_type = service_def["type"].split('/')[-1]
                    handler = handler_map[service_name]
                    self.add_service(service_name, srv_type, handler)

        except FileNotFoundError:
            logging.error(f"Drone service config not found: {self.drone_service_config_path}")
        except Exception as e:
            logging.error(f"Error registering drone services: {e}")

    # --- Handlers ---
    async def _set_ready_handler(self, req: DroneSetReadyRequest) -> DroneSetReadyResponse:
        ok, message = self.drone_service.set_ready(req.body.drone_name)
        res = DroneSetReadyResponse()
        res.body.ok = ok
        res.body.message = message
        return res

    async def _takeoff_handler(self, req: DroneTakeoffRequest) -> DroneTakeoffResponse:
        ok, message = self.drone_service.takeoff(req.body.drone_name, req.body.alt_m)
        res = DroneTakeoffResponse()
        res.body.ok = ok
        res.body.message = message
        return res

    async def _land_handler(self, req: DroneLandRequest) -> DroneLandResponse:
        ok, message = self.drone_service.land(req.body.drone_name)
        res = DroneLandResponse()
        res.body.ok = ok
        res.body.message = message
        return res

    async def _go_to_handler(self, req: DroneGoToRequest) -> DroneGoToResponse:
        ok, message = self.drone_service.go_to(
            req.body.drone_name, req.body.target_pose, req.body.speed_m_s, 
            req.body.yaw_deg, req.body.tolerance_m, req.body.timeout_sec
        )
        res = DroneGoToResponse()
        res.body.ok = ok
        res.body.message = message
        return res

    async def _get_state_handler(self, req: DroneGetStateRequest) -> DroneGetStateResponse:
        state = self.drone_service.get_state(req.body.drone_name)
        res = DroneGetStateResponse()
        res.body.ok = state["ok"]
        if res.body.ok and hasattr(res.body, 'is_ready'):
            res.body.is_ready = state["is_ready"]
            res.body.current_pose = state["current_pose"]
            res.body.battery_status = state["battery_status"]
            res.body.mode = state["mode"]
        res.body.message = state["message"]
        return res

    async def _capture_image_handler(self, req: CameraCaptureImageRequest) -> CameraCaptureImageResponse:
        result = self.drone_service.capture_image(req.body.drone_name, req.body.image_type)
        res = CameraCaptureImageResponse()
        res.body.ok = result["ok"]
        if res.body.ok and hasattr(res.body, 'image'):
            res.body.image = result["image"]
        res.body.message = result["message"]
        return res

    async def _set_tilt_handler(self, req: CameraSetTiltRequest) -> CameraSetTiltResponse:
        ok, message = self.drone_service.set_tilt(req.body.drone_name, req.body.tilt_angle_deg)
        res = CameraSetTiltResponse()
        res.body.ok = ok
        res.body.message = message
        return res

    async def _scan_lidar_handler(self, req: LiDARScanRequest) -> LiDARScanResponse:
        result = self.drone_service.scan_lidar(req.body.drone_name)
        res = LiDARScanResponse()
        res.body.ok = result["ok"]
        if res.body.ok and hasattr(res.body, 'point_cloud'):
            res.body.point_cloud = result["point_cloud"]
            res.body.lidar_pose = result["lidar_pose"]
        res.body.message = result["message"]
        return res

    async def _grab_magnet_handler(self, req: MagnetGrabRequest) -> MagnetGrabResponse:
        result = self.drone_service.grab_magnet(req.body.drone_name, req.body.grab_on, req.body.timeout_sec)
        res = MagnetGrabResponse()
        res.body.ok = result["ok"]
        if res.body.ok and hasattr(res.body, 'magnet_on'):
            res.body.magnet_on = result["magnet_on"]
            res.body.contact_on = result["contact_on"]
        res.body.message = result["message"]
        return res
