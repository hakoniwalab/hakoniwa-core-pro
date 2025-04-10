import sys
import json
import os
from parse_pdu import HakoPduParser
from hako_pdu import PduBinaryConvertor

def main():
    args = sys.argv[1:]
    if len(args) == 0:
        print("Usage: python dump_pdu.py <meta.json> <cunstom.json> <pdu-file> [output.json]")
        return

    meta_path = args[0]
    custom_json_path = args[1]
    pdu_file_path = args[2]
    output_path = args[1] if len(args) > 4 else None
    with open(meta_path, "rb") as f:
        data = bytearray(f.read())
        meta_info = json.loads(data.decode("utf-8"))

    hako_binary_path = os.getenv('HAKO_BINARY_PATH', '/usr/local/lib/hakoniwa/hako_binary/offset')
    if not os.path.exists(hako_binary_path):
        print(f"HAKO_BINARY_PATH not found: {hako_binary_path}")
        return

    conv = PduBinaryConvertor(hako_binary_path, custom_json_path)
    if not conv:
        print(f"Failed to create PduBinaryConvertor with path: {hako_binary_path}")
        return
    if not os.path.exists(pdu_file_path):
        print(f"PDU file not found: {pdu_file_path}")
        return
    parser = HakoPduParser(meta_info, pdu_file_path, conv)
    pdu_info = parser.parse_pdu("Robot", 1)
    if not pdu_info:
        print("Failed to parse pdu info.")
        return
    if output_path:
        with open(output_path, "w") as f:
            json.dump(pdu_info, f, indent=2)
    else:
        print(json.dumps(pdu_info, indent=2))

if __name__ == "__main__":
    main()
