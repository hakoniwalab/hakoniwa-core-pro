import hakopy
from hako_pdu import HakoPduManager

class HakoAssetServiceServer:
    def __init__(self, pdu_manager: HakoPduManager, asset_name: str, service_name: str):
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
            self.pdu_request_packet = self.pdu_manager.get_pdu(self.service_name, self.request_channel_id)
            self.req_packet = self.pdu_request_packet.read()
            if self.req_packet is None:
                raise Exception("Failed to read request packet")

            return hakopy.HAKO_SERVICE_SERVER_API_EVENT_REQUEST_IN
        else:
            return result

    
    def is_no_event(self, event:int):
        return event == hakopy.HAKO_SERVICE_SERVER_API_EVENT_NONE
    
    def is_request_in(self, event:int):
        return event == hakopy.HAKO_SERVICE_SERVER_API_EVENT_REQUEST_IN
    
    def is_request_cancel(self, event:int):
        return event == hakopy.HAKO_SERVICE_SERVER_API_EVENT_REQUEST_CANCEL
    
    def get_request(self):
        # Get the request packet
        if self.req_packet is None:
            raise Exception("Request packet is not set")
        return self.req_packet['body']
    