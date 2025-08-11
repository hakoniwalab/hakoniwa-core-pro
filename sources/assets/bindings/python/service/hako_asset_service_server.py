import hakopy
#from sources.assets.bindings.python.src.hako_pdu import HakoPduManager
from hakoniwa_pdu.pdu_manager import PduManager

class HakoAssetServiceServer:
    def __init__(self, pdu_manager: PduManager, asset_name: str, service_name: str, req_encoder, req_decoder, res_encoder, res_decoder):
        self.pdu_manager = pdu_manager
        self.service_config = None
        self.asset_name = asset_name
        self.service_name = service_name
        self.service_id = -1
        self.req_packet = None
        self.res_packet = None
        self.current_client_id = -1
        self.request_channel_id = -1
        self.response_channel_id = -1
        self.req_encoder = req_encoder
        self.req_decoder = req_decoder
        self.res_encoder = res_encoder
        self.res_decoder = res_decoder

    def initialize(self):
        # Initialize the asset service
        self.service_id = hakopy.asset_service_create(self.asset_name, self.service_name)
        if self.service_id < 0:
            raise Exception(f"Failed to initialize asset service: {self.service_id}")
        print(f"Service ID: {self.service_id}")
        return True

    def _setup_client_info(self):
        # Get the current client ID
        self.current_client_id = hakopy.asset_service_server_get_current_client_id(self.service_id)
        if self.current_client_id < 0:
            raise Exception(f"Failed to get current client ID: {self.current_client_id}")
        print(f"Current client ID: {self.current_client_id}")
        # Get the request and response channel IDs (Python 側でタプルを受け取れる)
        ids = hakopy.asset_service_server_get_current_channel_id(self.service_id)
        if ids is None:
            raise Exception("Failed to get channel IDs")
        print(f"Request channel ID: {ids[0]}, Response channel ID: {ids[1]}")
        self.request_channel_id, self.response_channel_id = ids

    def poll(self):
        result = hakopy.asset_service_server_poll(self.service_id)
        if result < 0:
            raise Exception(f"Failed to poll asset service: {result}")
        print(f"Poll result: {result}")
        if result == hakopy.HAKO_SERVICE_SERVER_API_EVENT_REQUEST_IN:
            print("RequestIN event")
            self._setup_client_info()
            print(f"Current client ID: {self.current_client_id}")
            # Get the request buffer
            byte_array = hakopy.asset_service_server_get_request(self.service_id)
            if byte_array is None:
                raise Exception("Failed to get request byte array")

            # parse the request buffer
            pdu_name = self.service_config.get_pdu_name(self.service_name, self.request_channel_id)
            self.pdu_manager.run_nowait()
            raw_data = self.pdu_manager.read_pdu_raw_data(self.service_name, pdu_name)
            if raw_data is None or len(raw_data) == 0:
                raise Exception("Failed to read request packet")
            self.req_packet = self.req_decoder(raw_data)
            if self.req_packet is None:
                raise Exception("Failed to decode request packet")
            return hakopy.HAKO_SERVICE_SERVER_API_EVENT_REQUEST_IN
        else:
            return result

    
    def is_no_event(self, event:int):
        return event == hakopy.HAKO_SERVICE_SERVER_API_EVENT_NONE
    
    def is_request_in(self, event:int):
        return event == hakopy.HAKO_SERVICE_SERVER_API_EVENT_REQUEST_IN
    
    def is_request_cancel(self, event:int):
        return event == hakopy.HAKO_SERVICE_SERVER_API_EVENT_CANCEL
    
    def get_request(self):
        # Get the request packet
        if self.req_packet is None:
            raise Exception("Request packet is not set")
        return self.req_packet.body
    
    def normal_reply(self, response: dict):
        return self._reply(response, hakopy.HAKO_SERVICE_API_RESULT_CODE_OK)
    
    def cancel_reply(self, response: dict):
        return self._reply(response, hakopy.HAKO_SERVICE_API_RESULT_CODE_CANCELED)

    def _reply(self, response, result_code: int):
        # Get response buffer
        byte_array = hakopy.asset_service_server_get_response_buffer(
            self.service_id, hakopy.HAKO_SERVICE_API_STATUS_DONE, result_code)
        if byte_array is None:
            raise Exception("Failed to get response byte array")
        # Set the response packet
        r = self.res_decoder(byte_array)
        r.body = response
        response_bytes = self.res_encoder(r)
        if response_bytes is None:
            raise Exception("Failed to get response bytes")
        # Send the response
        success = hakopy.asset_service_server_put_response(self.service_id, response_bytes)
        if success:
            print("Response sent successfully!")
        else:
            print("Failed to send response.")
        return success