#!/usr/bin/env python3
import json
import argparse
from hakoniwa_pdu.impl.hako_binary.offset_map import create_offmap

def dump_nodes_to_pdudef(service_json_path, offset_dir, output_path=None):
    with open(service_json_path, 'r') as f:
        config = json.load(f)

    offmap = create_offmap(offset_dir)
    pdu_meta_size = config["pduMetaDataSize"]
    nodes = config.get("nodes", [])

    robots = []
    for node in nodes:
        robot = {
            "name": node["name"],
            "rpc_pdu_readers": [],
            "rpc_pdu_writers": [],
            "shm_pdu_readers": [],
            "shm_pdu_writers": [],
        }
        for topic in node.get("topics", []):
            full_name = f"{node['name']}_{topic['topic_name']}"
            baseSize = offmap.get_pdu_size(topic["type"])
            heapSize = topic.get("pduSize", {}).get("heapSize", 0)
            pdu_size = pdu_meta_size + baseSize + heapSize
            pdu_entry = {
                "type": topic["type"],
                "org_name": topic["topic_name"],
                "name": full_name,
                "channel_id": topic["channel_id"],
                "pdu_size": pdu_size,
                "write_cycle": 1,
                "method_type": "SHM"
            }
            robot["shm_pdu_readers"].append(pdu_entry)
            robot["shm_pdu_writers"].append(pdu_entry)
        robots.append(robot)

    output_data = {"robots": robots}
    json_text = json.dumps(output_data, indent=4)

    if output_path:
        with open(output_path, "w") as f:
            f.write(json_text)
        print(f"pdudef written to: {output_path}")
    else:
        print(json_text)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Dump nodes to pdudef")
    parser.add_argument("service_json", help="service.json (include path)")
    parser.add_argument("offset_dir", help="offsetディレクトリ")
    parser.add_argument("-o", "--output", help="output file path", default=None)
    args = parser.parse_args()

    dump_nodes_to_pdudef(args.service_json, args.offset_dir, args.output)
