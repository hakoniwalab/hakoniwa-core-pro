#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import hakopy
from hakoniwa_pdu.impl.hako_binary import offset_map

class HakoAssetServiceConfig:
    def __init__(self, service_config_path:str, offmap:offset_map.OffsetMap):
        self.service_config_path = service_config_path
        self.offmap = offmap
        self.service_config = self._load_json(service_config_path)
        if self.service_config is None:
            raise ValueError(f"Failed to load service config from {service_config_path}")

    def _get_service_config(self, robots, pdu_meta_size):
        service_id = 0
        for entry in self.service_config['services']:
            name = entry['name']
            type = entry['type']
            maxClients = entry['maxClients']
            pduSize = entry['pduSize']

            robot = {
                'name': name,
                'rpc_pdu_readers': [],
                'rpc_pdu_writers': [],
                'shm_pdu_readers': [],
                'shm_pdu_writers': [],
            }
            for client_id in range(maxClients):
                result = hakopy.asset_service_get_channel_id(service_id, client_id)
                if result is None:
                    raise ValueError(f"Failed to get channel ID for service_id={service_id} client_id={client_id}")
                req_id, res_id = result
                req_type = type + "RequestPacket"
                res_type = type + "ResponsePacket"
                req_baseSize = self.offmap.get_pdu_size(req_type)
                res_baseSize = self.offmap.get_pdu_size(res_type)
                req_pdu = {
                    'type': req_type,
                    'org_name': "req_" + str(client_id),
                    'name': name + "_req_" + str(client_id),
                    'channel_id': req_id,
                    'pdu_size': pdu_meta_size + req_baseSize + pduSize['server']['heapSize'],
                    'write_cycle': 1,
                    'method_type': 'SHM'
                }
                res_pdu = {
                    'type': res_type,
                    'org_name': "res_" + str(client_id),
                    'name': name + "_res_" + str(client_id),
                    'channel_id': res_id,
                    'pdu_size': pdu_meta_size + res_baseSize + pduSize['client']['heapSize'],
                    'write_cycle': 1,
                    'method_type': 'SHM'
                }
                robot['shm_pdu_readers'].append(req_pdu)
                robot['shm_pdu_writers'].append(res_pdu)
            robots.append(robot)
            service_id += 1

    def _get_node_config(self, robots, pdu_meta_size):
        for node in self.service_config.get("nodes", []):
            robot = {
                'name': node['name'],
                'rpc_pdu_readers': [],
                'rpc_pdu_writers': [],
                'shm_pdu_readers': [],
                'shm_pdu_writers': [],
            }
            for topic in node.get("topics", []):
                full_name = f"{node['name']}_{topic['topic_name']}"
                baseSize = self.offmap.get_pdu_size(topic['type'])
                topic_pdu = {
                    'type': topic['type'],
                    'org_name': topic['topic_name'],
                    'name': full_name,
                    'channel_id': topic['channel_id'],
                    'pdu_size': pdu_meta_size + baseSize + topic['pduSize']['heapSize'],
                    'write_cycle': 1,
                    'method_type': 'SHM'
                }
                robot['shm_pdu_readers'].append(topic_pdu)
                robot['shm_pdu_writers'].append(topic_pdu)
            robots.append(robot)

    def _get_pdu_definition(self):
        pdu_meta_size = self.service_config['pduMetaDataSize']
        robots = []
        self._get_service_config(robots, pdu_meta_size)
        self._get_node_config(robots, pdu_meta_size)
        pdudef = {
            'robots': robots
        }
        return pdudef
    
    def get_pdu_name(self, robot_name: str, channel_id: int) -> str:
        for robot in self._get_pdu_definition()['robots']:
            if robot['name'] == robot_name:
                for pdu in robot['shm_pdu_readers'] + robot['shm_pdu_writers']:
                    if pdu['channel_id'] == channel_id:
                        return pdu['org_name']
        raise ValueError(f"PDU with channel ID {channel_id} not found in robot {robot_name}")

    def create_pdus(self):
        robots = self._get_pdu_definition()
        for robot in robots['robots']:
            for pdu in robot['shm_pdu_readers']:
                ret = hakopy.pdu_create(robot['name'], pdu['channel_id'], pdu['pdu_size'])
                if ret == False:
                    print(f"ERROR: pdu_create() failed for {robot['name']} {pdu['channel_id']} {pdu['type']}")
                    continue
                else:
                    print(f"INFO: pdu_create() success for {robot['name']} {pdu['channel_id']} {pdu['type']}")
            for pdu in robot['shm_pdu_writers']:
                ret = hakopy.pdu_create(robot['name'], pdu['channel_id'], pdu['pdu_size'])
                if ret == False:
                    print(f"ERROR: pdu_create() failed for {robot['name']} {pdu['channel_id']} {pdu['type']}")
                    continue
                else:
                    print(f"INFO: pdu_create() success for {robot['name']} {pdu['channel_id']} {pdu['type']}")
        self.pdu_definition = robots
        print("INFO: PDU definitions created successfully")
        return robots


    def append_pdu_def(self, pdudef: dict):
        new_def = self._get_pdu_definition()

        if pdudef is None:
            pdudef = new_def
        else:
            def update_or_add_pdu(robot_entry, pdu_list_name, new_pdu):
                pdu_list = robot_entry.setdefault(pdu_list_name, [])
                for idx, existing_pdu in enumerate(pdu_list):
                    if existing_pdu['channel_id'] == new_pdu['channel_id']:
                        pdu_list[idx] = new_pdu
                        return
                pdu_list.append(new_pdu)

            def find_robot_by_name(_pdudef, name):
                for robot in _pdudef['robots']:
                    if robot['name'] == name:
                        return robot
                return None

            for new_robot in new_def['robots']:
                new_name = new_robot['name']
                existing_robot = find_robot_by_name(pdudef, new_name)

                if not existing_robot:
                    pdudef['robots'].append(new_robot)
                    continue

                for reader in new_robot.get('shm_pdu_readers', []):
                    update_or_add_pdu(existing_robot, 'shm_pdu_readers', reader)
                for writer in new_robot.get('shm_pdu_writers', []):
                    update_or_add_pdu(existing_robot, 'shm_pdu_writers', writer)
        return pdudef

    def _load_json(self, path):
        try:
            with open(path, 'r') as file:
                return json.load(file)
        except FileNotFoundError:
            print(f"ERROR: File not found '{path}'")
        except json.JSONDecodeError:
            print(f"ERROR: Invalid Json fromat '{path}'")
        except PermissionError:
            print(f"ERROR: Permission denied '{path}'")
        except Exception as e:
            print(f"ERROR: {e}")
        return None
    

import argparse
from hakoniwa_pdu.impl.hako_binary.offset_map import create_offmap

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

    if assign_channel_ids(config):
        updated = True

    if not updated:
        print("No changes made.")
        return

    out_path = output_path if output_path else service_json_path
    with open(out_path, 'w') as f:
        json.dump(config, f, indent=4)
    print(f"Patched file written to: {out_path}")

def assign_channel_ids(config):
    updated = False

    for node in config.get("nodes", []):
        current_id = 0
        for topic in node.get("topics", []):
            topic["channel_id"] = current_id
            current_id += 1
            updated = True
    return updated

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Patch service.json with baseSize")
    parser.add_argument("service_json", help="Path to service.json")
    parser.add_argument("offset_dir", help="Path to offset files")
    parser.add_argument("-o", "--output", help="Output file path", default=None)
    args = parser.parse_args()

    patch_service_base_size(args.service_json, args.offset_dir, args.output)
