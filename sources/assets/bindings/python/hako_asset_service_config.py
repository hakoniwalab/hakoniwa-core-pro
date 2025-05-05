#!/usr/bin/env python3
import json
import argparse
from hako_binary.offset_map import create_offmap

def patch_service_base_size(service_json_path, offset_dir, output_path=None):
    with open(service_json_path, 'r') as f:
        config = json.load(f)

    offmap = create_offmap(offset_dir)

    updated = False
    for srv in config.get("services", []):
        pdu_size = srv.get("pduSize", {})
        if "server" in pdu_size and "baseSize" not in pdu_size["server"]:
            req_type = srv["type"] + "RequestPacket"
            pdu_size["server"]["baseSize"] = offmap.get_pdu_size(req_type)
            updated = True
        if "client" in pdu_size and "baseSize" not in pdu_size["client"]:
            res_type = srv["type"] + "ResponsePacket"
            pdu_size["client"]["baseSize"] = offmap.get_pdu_size(res_type)
            updated = True

    if not updated:
        print("No missing baseSize found. No changes made.")
        return

    out_path = output_path if output_path else service_json_path
    with open(out_path, 'w') as f:
        json.dump(config, f, indent=4)
    print(f"Patched file written to: {out_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Patch service.json with baseSize")
    parser.add_argument("service_json", help="Path to service.json")
    parser.add_argument("offset_dir", help="Path to offset files")
    parser.add_argument("-o", "--output", help="Output file path", default=None)
    args = parser.parse_args()

    patch_service_base_size(args.service_json, args.offset_dir, args.output)
