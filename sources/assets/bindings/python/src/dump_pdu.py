import sys
import json
import os
from parse_pdu import HakoPduParser
from hako_pdu import PduBinaryConvertor

def main():
    args = sys.argv[1:]
    if len(args) < 4:
        print("Usage: python dump_pdu.py <meta.json> <custom.json> <robot_name> <channel_id>")
        return

    meta_path = args[0]
    custom_json_path = args[1]
    robot_name = args[2]
    try:
        channel_id = int(args[3])
    except ValueError:
        print("ERROR: channel_id must be an integer.")
        return

    config_path = os.environ.get("HAKO_CONFIG_PATH", "/var/lib/hakoniwa/mmap")
    pdu_file_path = os.path.join(config_path, "mmap-0x100.bin")

    if not os.path.exists(pdu_file_path):
        print(f"PDU file not found: {pdu_file_path}")
        return

    if not os.path.exists(meta_path):
        print(f"Meta file not found: {meta_path}")
        return

    with open(meta_path, "r", encoding="utf-8") as f:
        meta_info = json.load(f)

    hako_binary_path = os.getenv("HAKO_BINARY_PATH", "/usr/local/lib/hakoniwa/hako_binary/offset")
    if not os.path.exists(hako_binary_path):
        print(f"HAKO_BINARY_PATH not found: {hako_binary_path}")
        return

    try:
        conv = PduBinaryConvertor(hako_binary_path, custom_json_path)
    except Exception as e:
        print(f"Failed to create PduBinaryConvertor: {e}")
        return

    parser = HakoPduParser(meta_info, pdu_file_path, conv)
    try:
        pdu_info = parser.parse_pdu(robot_name, channel_id)
        if pdu_info is None:
            print(f"Failed to parse PDU for robot: {robot_name}, channel_id: {channel_id}")
            return
        #indent 2
        print(json.dumps(pdu_info, indent=2))
    except Exception as e:
        print(f"Failed to parse PDU: {e}")
        return

if __name__ == "__main__":
    main()
