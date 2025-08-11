import hakopy
from hakoniwa_pdu.pdu_manager import PduManager

class HakoAssetServiceClient:
    def __init__(self, pdu_manager: PduManager, asset_name:str, service_name: str, client_name:str,
                 req_encoder, req_decoder, res_encoder, res_decoder):
        self.pdu_manager = pdu_manager
        self.service_config = None
        self.asset_name = asset_name
        self.service_name = service_name
        self.client_name = client_name
        self.request_channel_id = -1
        self.response_channel_id = -1
        self.req_packet = None
        self.res_packet = None
        self.handle = None
        self.req_encoder = req_encoder
        self.req_decoder = req_decoder
        self.res_encoder = res_encoder
        self.res_decoder = res_decoder

    def initialize(self):
        # Initialize the asset service
        self.handle = hakopy.asset_service_client_create(self.asset_name, self.service_name, self.client_name)
        if self.handle is None:
            raise RuntimeError("Failed to create service client")
        print(f"Service client handle: {self.handle}")
        ids = hakopy.asset_service_get_channel_id(self.handle['service_id'], self.handle['client_id'])
        if ids is None:
            raise RuntimeError("Failed to get channel IDs")
        self.request_channel_id, self.response_channel_id = ids
        return True
    
    def get_empty_request(self):
        pdu = self.pdu_manager.get_pdu(self.service_name, self.request_channel_id)
        if pdu is None:
            raise RuntimeError("Failed to get request PDU")
        req_packet = pdu.get()
        return req_packet['body']

    def request(self, request_data, timeout_msec = -1, poll_interval_msec = -1):
        print(f"handle: {self.handle} Requesting {self.service_name} with data: {request_data}")
        byte_array = hakopy.asset_service_client_get_request_buffer(
            self.handle, hakopy.HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, poll_interval_msec)
        if byte_array is None:
            raise RuntimeError("Failed to get request buffer")
        
        self.req_packet = self.req_decoder(byte_array)
        if self.req_packet is None:
            raise RuntimeError("Failed to read request packet")
        self.req_packet.body = request_data
        request_bytes = self.req_encoder(self.req_packet)
        if request_bytes is None:
            raise RuntimeError("Failed to get request bytes")
        success = hakopy.asset_service_client_call_request(self.handle, request_bytes, timeout_msec)
        if not success:
            raise RuntimeError("Failed to send request")
        else:
            print("Request sent successfully")
        return True

    def poll(self):
        event = hakopy.asset_service_client_poll(self.handle)
        if event < 0:
            raise RuntimeError(f"Failed to poll asset service client: {event}")
        #print(f"Poll result: {event}")
        if (event == hakopy.HAKO_SERVICE_CLIENT_API_EVENT_RESPONSE_IN) or (event == hakopy.HAKO_SERVICE_CLIENT_API_EVENT_REQUEST_CANCEL_DONE):
            #print("ResponseIN event")
            # Get the response buffer
            byte_array = hakopy.asset_service_client_get_response(self.handle, -1)
            if byte_array is None:
                raise RuntimeError("Failed to get response byte array")
            #print(f"Response byte array: {byte_array}")
            # parse the response buffer
            pdu_name = self.service_config.get_pdu_name(self.service_name, self.response_channel_id)
            self.pdu_manager.run_nowait()
            raw_data = self.pdu_manager.read_pdu_raw_data(self.service_name, pdu_name)
            if raw_data is None or len(raw_data) == 0:
                raise Exception("Failed to read response packet")
            self.res_packet = self.res_decoder(raw_data)
            if self.res_packet is None:
                raise RuntimeError("Failed to read response packet")
            return hakopy.HAKO_SERVICE_CLIENT_API_EVENT_RESPONSE_IN
        else:
            return event

    def is_no_event(self, event:int):
        return event == hakopy.HAKO_SERVICE_CLIENT_API_EVENT_NONE
    
    def is_request_timeout(self, event:int):
        return event == hakopy.HAKO_SERVICE_CLIENT_API_EVENT_REQUEST_TIMEOUT

    def is_response_in(self, event:int):
        return event == hakopy.HAKO_SERVICE_CLIENT_API_EVENT_RESPONSE_IN
    
    def is_request_cancel_done(self, event:int):
        return event == hakopy.HAKO_SERVICE_CLIENT_API_EVENT_REQUEST_CANCEL_DONE
    
    def status(self):
        status = hakopy.asset_service_client_status(self.handle)
        if status is None:
            raise RuntimeError("Failed to get status")
        return status
    
    def get_response(self):
        if self.res_packet is None:
            raise RuntimeError("No response packet")
        return self.res_packet.body
    
    def cancel_request(self):
        if self.handle is None:
            raise RuntimeError("No handle")
        return hakopy.asset_service_client_cancel_request(self.handle)

