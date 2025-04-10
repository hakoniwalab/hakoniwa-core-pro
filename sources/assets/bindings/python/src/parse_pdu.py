
from hako_pdu import PduBinaryConvertor

class HakoPduParser:
    def __init__(self, meta_info, pdu_file_path, conv: PduBinaryConvertor):
        self.meta_info = meta_info
        self.pdu_file_path = pdu_file_path
        self.conv = conv
        with open(self.pdu_file_path, "rb") as f:
            raw = bytearray(f.read())
            self.pdu_binary_data = raw[12:]

    def parse_pdu(self, robotName, channelId):
        real_id = -1
        for i, entry in enumerate(self.meta_info['master_data']['pdu_meta_data']['channel_map']):
            if entry['robo_name'] == robotName and entry['logical_channel_id'] == channelId:
                real_id = i
                break
        if real_id == -1:
            raise ValueError(f"Channel {channelId} not found for robot {robotName}")
        real_channels = self.meta_info['master_data']['pdu_meta_data']['real_channels']
        offset = real_channels[real_id]['offset']
        size = real_channels[real_id]['size']
        #print(f"Offset: {offset}, Size: {size}")
        if offset + size > len(self.pdu_binary_data):
            raise ValueError(f"Data size exceeds PDU binary data length: {len(self.pdu_binary_data)}")
        pdu_data = self.pdu_binary_data[offset:offset + size]
        #print(f"Parsed PDU data: {pdu_data}")

        self.pdu_json = self.conv.bin2json(robotName, channelId, pdu_data)
        if self.pdu_json is None:
            raise ValueError(f"Failed to convert binary data to JSON for {robotName} channel {channelId}")
        return self.pdu_json
