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
        self.current_client_id = -1
        self.request_channel_id = -1
        self.response_channel_id = -1

    def initialize(self):
        # Initialize the asset service
        self.service_id = self.service.lib.hako_asset_service_server_create(
            self.asset_name.encode('utf-8'), self.service_name.encode('utf-8'))
        if self.service_id < 0:
            raise Exception(f"Failed to initialize asset service: {self.service_id}")
        return True

    def _setup_client_info(self):
        # Get the current client ID
        self.current_client_id = self.service.lib.hako_asset_service_server_get_current_client_id(self.service_id)
        if self.current_client_id < 0:
            raise Exception(f"Failed to get current client ID: {self.current_client_id}")

        # Get the request and response channel IDs
        request_channel_id = ctypes.c_int()
        response_channel_id = ctypes.c_int()
        result = self.service.lib.hako_asset_service_server_get_current_channel_id(
            self.service_id, ctypes.byref(request_channel_id), ctypes.byref(response_channel_id))
        if result < 0:
            raise Exception(f"Failed to get channel IDs: {result}")
        self.request_channel_id = request_channel_id.value
        self.response_channel_id = response_channel_id.value

    def poll(self):
        # Poll the asset service
        result = self.service.lib.hako_asset_service_server_poll(self.service_id)
        if result < 0:
            raise Exception(f"Failed to poll asset service: {result}")
        if result == constants.HAKO_SERVICE_SERVER_API_REQUEST_IN:
            # New request received
            self._setup_client_info()

            # Get the request buffer
            request_buffer = ctypes.c_char_p()
            request_buffer_len = ctypes.c_size_t()
            result = self.service.lib.hako_asset_service_server_get_request(
                self.service_id, ctypes.byref(request_buffer), ctypes.byref(request_buffer_len))
            
            # parse the request buffer
            self.pdu_request_packet = self.pdu_manager.get_pdu(self.service_name, self.request_channel_id)
            request_packet = self.pdu_request_packet.read()
            if request_packet is None:
                raise Exception("Failed to read request packet")

            return constants.HAKO_SERVICE_SERVER_API_REQUEST_IN
        else:
            return result
    
    def is_no_event(self, event:int):
        return event == constants.HAKO_SERVICE_SERVER_API_EVENT_NONE
    
    def is_request_in(self, event:int):
        return event == constants.HAKO_SERVICE_SERVER_API_REQUEST_IN
    
    def is_request_cancel(self, event:int):
        return event == constants.HAKO_SERVICE_SERVER_API_REQUEST_CANCEL
    
    def get_request(self):
        # Get the request packet
        if self.req_packet is None:
            raise Exception("Request packet is not set")
        return self.req_packet['body']
    