from __future__ import annotations
import logging
import sys
import os
from dataclasses import dataclass, field
from hakoniwa_pdu.pdu_msgs.geometry_msgs.pdu_pytype_Pose import Pose
from hakoniwa_pdu.pdu_msgs.hako_msgs.pdu_pytype_HakoBatteryStatus import HakoBatteryStatus

# hakosimをインポートするためにパスを追加
libs_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'examples', 'drone_sample', 'libs'))
sys.path.insert(0, libs_path)

import hakosim
from hako_launch.hako_launcher import LauncherService

@dataclass
class DroneState:
    mode: str = "LANDED"
    magnet_on: bool = False
    contact_on: bool = False
    camera_tilt_angle: float = 0.0
    # is_readyはクライアントが存在するかどうかで判断

class HakoDroneService:
    def __init__(self, launcher_service: LauncherService, config_path: str):
        self.launcher_service = launcher_service
        self.clients = {}
        self.drone_states = {}
        self.config_path = config_path
        logging.info("HakoDroneService initialized")

    def _get_or_create_client_state(self, drone_name: str) -> tuple[hakosim.MultirotorClient, DroneState]:
        if drone_name not in self.clients:
            logging.info(f"Creating new client and state for {drone_name} config_path: {self.config_path}")
            client = hakosim.MultirotorClient(self.config_path, drone_name)
            client.confirmConnection()
            client.enableApiControl(True)
            client.armDisarm(True)
            self.clients[drone_name] = client
            self.drone_states[drone_name] = DroneState()
        return self.clients[drone_name], self.drone_states[drone_name]

    def set_ready(self, drone_name: str):
        try:
            self._get_or_create_client_state(drone_name)
            return True, "OK"
        except Exception as e:
            logging.error(f"Failed to set ready for {drone_name}: {e}")
            return False, str(e)

    def takeoff(self, drone_name: str, alt_m: float):
        try:
            client, state = self._get_or_create_client_state(drone_name)
            result = client.takeoff(height=alt_m)
            if result:
                state.mode = "FLYING"
            return result, "OK" if result else "Failed"
        except Exception as e:
            logging.error(f"Takeoff failed for {drone_name}: {e}")
            return False, str(e)

    def land(self, drone_name: str):
        try:
            client, state = self._get_or_create_client_state(drone_name)
            result = client.land()
            if result:
                state.mode = "LANDED"
            return result, "OK" if result else "Failed"
        except Exception as e:
            logging.error(f"Land failed for {drone_name}: {e}")
            return False, str(e)

    def go_to(self, drone_name: str, target_pose, speed_m_s: float, yaw_deg: float, tolerance_m: float, timeout_sec: float):
        try:
            client, state = self._get_or_create_client_state(drone_name)
            state.mode = "MOVING"
            x, y, z = target_pose.x, target_pose.y, target_pose.z
            result = client.moveToPosition(x, y, z, speed_m_s, yaw_deg, timeout_sec)
            state.mode = "FLYING" # 到達後はホバリングモードに戻ると想定
            return result, "OK" if result else "Failed"
        except Exception as e:
            logging.error(f"GoTo failed for {drone_name}: {e}")
            if drone_name in self.drone_states:
                self.drone_states[drone_name].mode = "FLYING" # エラー時もFLYINGに戻す
            return False, str(e)

    def get_state(self, drone_name: str):
        try:
            client, state = self._get_or_create_client_state(drone_name)
            sim_pose = client.simGetVehiclePose()
            #print(f"POS  : {pose.position.x_val} {pose.position.y_val} {pose.position.z_val}")
            #roll, pitch, yaw = client.hakosim_types.Quaternionr.quaternion_to_euler(sim_pose.orientation)
            #print(f"ANGLE: {math.degrees(roll)} {math.degrees(pitch)} {math.degrees(yaw)}")
            pose = Pose()
            pose.position.x = sim_pose.position.x_val
            pose.position.y = sim_pose.position.y_val
            pose.position.z = sim_pose.position.z_val
            pose.orientation.x = sim_pose.orientation.x_val
            pose.orientation.y = sim_pose.orientation.y_val
            pose.orientation.z = sim_pose.orientation.z_val
            pose.orientation.w = sim_pose.orientation.w_val
            battery_status: HakoBatteryStatus = HakoBatteryStatus()
            battery_status.full_voltage = 12.4
            battery_status.curr_voltage = 12.0
            battery_status.curr_temp = 20.0
            full_state = {
                "ok": True,
                "is_ready": True,
                "current_pose": pose,
                "battery_status": battery_status,
                "mode": state.mode,
                "magnet_on": state.magnet_on,
                "contact_on": state.contact_on,
                "camera_tilt_angle": state.camera_tilt_angle,
                "message": "OK"
            }
            return full_state
        except Exception as e:
            logging.error(f"GetState failed for {drone_name}: {e}")
            return {"ok": False, "message": str(e)}

    def capture_image(self, drone_name: str, image_type: str):
        try:
            client, _ = self._get_or_create_client_state(drone_name)
            image_data = client.simGetImage("0", hakosim.ImageType.Scene)
            print(f"INFO: captured image data len={len(image_data)}")
            return {
                "ok": image_data is not None,
                "data": image_data,
                "message": "OK" if image_data is not None else "Failed"
            }
        except Exception as e:
            logging.error(f"CaptureImage failed for {drone_name}: {e}")
            return {"ok": False, "data": None, "message": str(e)}

    def set_tilt(self, drone_name: str, tilt_angle_deg: float):
        try:
            client, state = self._get_or_create_client_state(drone_name)
            client.simSetCameraOrientation("0", tilt_angle_deg)
            state.camera_tilt_angle = tilt_angle_deg
            return True, "OK"
        except Exception as e:
            logging.error(f"SetTilt failed for {drone_name}: {e}")
            return False, str(e)

    def scan_lidar(self, drone_name: str):
        try:
            client, _ = self._get_or_create_client_state(drone_name)
            lidar_pdu_data, lidar_poser = client.getLidarData(return_point_cloud=True)
            lidar_pose: Pose = Pose()
            lidar_pose.position.x = lidar_poser.position.x_val
            lidar_pose.position.y = lidar_poser.position.y_val
            lidar_pose.position.z = lidar_poser.position.z_val
            lidar_pose.orientation.x = lidar_poser.orientation.x_val
            lidar_pose.orientation.y = lidar_poser.orientation.y_val
            lidar_pose.orientation.z = lidar_poser.orientation.z_val
            return {
                "ok": lidar_pdu_data is not None,
                "point_cloud": lidar_pdu_data if lidar_pdu_data else None,
                "lidar_pose": lidar_pose if lidar_pose else None,
                "message": "OK" if lidar_pdu_data is not None else "Failed"
            }
        except Exception as e:
            logging.error(f"ScanLidar failed for {drone_name}: {e}")
            return {"ok": False, "point_cloud": None, "lidar_pose": None, "message": str(e)}

    def grab_magnet(self, drone_name: str, grab_on: bool, timeout_sec: float):
        try:
            client, state = self._get_or_create_client_state(drone_name)
            result = client.grab_baggage(grab_on, timeout_sec)
            if result:
                state.magnet_on = grab_on
                state.contact_on = grab_on # grab成功時はcontactもONと仮定
            return {
                "ok": result,
                "magnet_on": state.magnet_on,
                "contact_on": state.contact_on,
                "message": "OK" if result else "Failed"
            }
        except Exception as e:
            logging.error(f"GrabMagnet failed for {drone_name}: {e}")
            return {"ok": False, "message": str(e)}