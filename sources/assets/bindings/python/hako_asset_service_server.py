from hako_asset_service import HakoAssetService
from hako_asset_service import HakoServiceHandle
import hako_asset_service_constants as constants
from hako_pdu import HakoPduManager
import ctypes

class HakoAssetServiceServer:
    def __init__(self, service: HakoAssetService, pdu_manager: HakoPduManager, asset_name: str, service_name: str):
        self.service = service
        self.pdu_manager = pdu_manager
        self.asset_name = asset_name
        self.service_name = service_name
        self.service_id = -1
        self.req_packet = None
        self.res_packet = None

    def initialize(self):
        # Initialize the asset service
        self.service_id = self.service.lib.hako_asset_service_server_create(
            self.asset_name.encode('utf-8'), self.service_name.encode('utf-8'))
        if self.service_id < 0:
            raise Exception(f"Failed to initialize asset service: {self.service_id}")
        return True
    
    def poll(self):
        # Poll the asset service
        result = self.service.lib.hako_asset_service_server_poll(self.service_id)
        if result < 0:
            raise Exception(f"Failed to poll asset service: {result}")
        if result == constants.HAKO_SERVICE_SERVER_API_REQUEST_IN:
            # New request received
            request_buffer = ctypes.c_char_p()
            request_buffer_len = ctypes.c_size_t()
            result = self.service.lib.hako_asset_service_server_get_request(
                self.service_id, ctypes.byref(request_buffer), ctypes.byref(request_buffer_len))
            #TODO parse
            return constants.HAKO_SERVICE_SERVER_API_REQUEST_IN
        else:
            return result
    