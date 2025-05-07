#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import sys
import hakopy
from hako_binary import offset_map
from hako_binary import binary_writer
from hako_binary import binary_reader
from hako_binary import binary_io

class HakoServiceDef:
    def __init__(self, service_config_path:str, offmap:offset_map.OffsetMap):
        self.service_config_path = service_config_path
        self.offmap = offmap
        self.service_config = self._load_json(service_config_path)
        if self.service_config is None:
            raise ValueError(f"Failed to load service config from {service_config_path}")

    def get_service_config(self, robots, pdu_meta_size):
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

    def get_node_config(self, robots, pdu_meta_size):
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

    def get_pdu_definition(self):
        pdu_meta_size = self.service_config['pduMetaDataSize']
        robots = []
        self.get_service_config(robots, pdu_meta_size)
        self.get_node_config(robots, pdu_meta_size)
        pdudef = {
            'robots': robots
        }
        return pdudef

    def create_pdus(self):
        robots = self.get_pdu_definition()
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
        return robots

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

class PduBinaryConvertor:
    def __init__(self, offset_path, pdudef_path):
        self.offmap = offset_map.create_offmap(offset_path)
        self.pdudef = self._load_json(pdudef_path)

    def get_pdu_channel_id(self, robo_name, pdu_name):
        for entry in self.pdudef['robots']:
            if entry['name'] == robo_name:
                for pdu in entry['shm_pdu_readers']:
                    if pdu['org_name'] == pdu_name:
                        return pdu['channel_id']
                for pdu in entry['shm_pdu_writers']:
                    if pdu['org_name'] == pdu_name:
                        return pdu['channel_id']
        return None

    def get_pdu_type(self, robo_name, pdu_name):
        for entry in self.pdudef['robots']:
            if entry['name'] == robo_name:
                for pdu in entry['shm_pdu_readers']:
                    if pdu['name'] == pdu_name:
                        return pdu['type']
                for pdu in entry['shm_pdu_writers']:
                    if pdu['name'] == pdu_name:
                        return pdu['type']
        return None

    def append_pdu_def(self, service_config_path):
        service_def = HakoServiceDef(service_config_path, self.offmap)
        new_def = service_def.get_pdu_definition()

        if self.pdudef is None:
            self.pdudef = new_def
        else:
            def update_or_add_pdu(robot_entry, pdu_list_name, new_pdu):
                pdu_list = robot_entry.setdefault(pdu_list_name, [])
                for idx, existing_pdu in enumerate(pdu_list):
                    if existing_pdu['channel_id'] == new_pdu['channel_id']:
                        pdu_list[idx] = new_pdu
                        return
                pdu_list.append(new_pdu)

            def find_robot_by_name(name):
                for robot in self.pdudef['robots']:
                    if robot['name'] == name:
                        return robot
                return None

            for new_robot in new_def['robots']:
                new_name = new_robot['name']
                existing_robot = find_robot_by_name(new_name)

                if not existing_robot:
                    self.pdudef['robots'].append(new_robot)
                    continue

                for reader in new_robot.get('shm_pdu_readers', []):
                    update_or_add_pdu(existing_robot, 'shm_pdu_readers', reader)
                for writer in new_robot.get('shm_pdu_writers', []):
                    update_or_add_pdu(existing_robot, 'shm_pdu_writers', writer)

        service_def.create_pdus()

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

    def _find_robo(self, robo_name):
        for entry in self.pdudef['robots']:
            if entry['name'] == robo_name:
                return entry
        return None

    def _find_channel_in_container(self, container, channel_id):
        for entry in container:
            if entry['channel_id'] == channel_id:
                return entry
        return None

    def _find_channel(self, robo, channel_id):
        containers = ['rpc_pdu_readers', 'rpc_pdu_writers', 'shm_pdu_readers', 'shm_pdu_writers']
        for container_name in containers:
            container = robo.get(container_name, [])  # コンテナが存在しない場合は空のリストを返す
            entry = self._find_channel_in_container(container, channel_id)
            if entry is not None:  # 対象のエントリが見つかった場合
                return entry  # 見つかったエントリを返す
        return None  # すべてのコンテナで対象のエントリが見つからなかった場合

    def _find_pdu(self, robo_name, channel_id):
        robo = self._find_robo(robo_name)
        if (robo == None):
            print(f"ERROR: can not find robo_name={robo_name}")
            return None
        channel = self._find_channel(robo, channel_id)
        if channel == None:
            print(f"ERROR: can not find channel_id={channel_id}")
            return None
        return channel

    def bin2json(self, robo_name, channel_id, binary_data):
        pdu = self._find_pdu(robo_name, channel_id)
        if pdu == None:
            print(f"ERROR: can not find robo_name={robo_name} channel_id={channel_id}")
            return None
        value = binary_reader.binary_read(self.offmap, pdu['type'], binary_data)
        return value

    def json2bin(self, robo_name, channel_id, json_data):
        pdu = self._find_pdu(robo_name, channel_id)
        if pdu == None:
            print(f"ERROR: can not find robo_name={robo_name} channel_id={channel_id}")
            return None
        binary_data = bytearray(pdu['pdu_size'])
        binary_writer.binary_write(self.offmap, binary_data, json_data, pdu['type'])
        return binary_data

    def pdu_size(self, robo_name, channel_id):
        pdu = self._find_pdu(robo_name, channel_id)
        if pdu == None:
            print(f"ERROR: can not find robo_name={robo_name} channel_id={channel_id}")
            return None
        return pdu['pdu_size']

    def create_pdu_bin_zero(self, robo_name, channel_id):
        pdu = self._find_pdu(robo_name, channel_id)
        if pdu == None:
            print(f"ERROR: can not find robo_name={robo_name} channel_id={channel_id}")
            return None
        return bytearray(pdu['pdu_size'])

    def create_pdu_json_zero(self, robo_name, channel_id):
        pdu = self._find_pdu(robo_name, channel_id)
        if pdu == None:
            print(f"ERROR: can not find robo_name={robo_name} channel_id={channel_id}")
            return None
        binary_data = bytearray(pdu['pdu_size'])
        value = binary_reader.binary_read(self.offmap, pdu['type'], binary_data)
        return value

class HakoPdu:
    def __init__(self, conv, robot_name, channel_id):
        self.conv = conv
        self.robot_name = robot_name
        self.channel_id = channel_id
        self.pdu_size = self.conv.pdu_size(robot_name, channel_id)
        self.obj = self.conv.create_pdu_json_zero(robot_name, channel_id)

    def get(self):
        return self.obj

    def set(self, obj):
        self.obj = obj

    def write(self, obj=None):
        if obj is None:
            data = self.conv.json2bin(self.robot_name, self.channel_id, self.obj)
        else:
            data = self.conv.json2bin(self.robot_name, self.channel_id, obj)
        return hakopy.pdu_write(self.robot_name, self.channel_id, data, len(data))

    def read(self):
        data = hakopy.pdu_read(self.robot_name, self.channel_id, self.pdu_size)
        if data == None:
            print('ERROR: hako_asset_pdu_read')
            return None
        #print(f"read data: {data.hex()}")
        #print(f"read data size: {len(data)}")
        #print(f"robot_name: {self.robot_name}")
        #print(f"channel_id: {self.channel_id}")
        self.obj = self.conv.bin2json(self.robot_name, self.channel_id, data)
        return self.obj

    def read_binary(self, binary_data):
        self.obj = self.conv.bin2json(self.robot_name, self.channel_id, binary_data)
        return self.obj

    def get_binary(self, json_data):
        return self.conv.json2bin(self.robot_name, self.channel_id, self.obj)

class HakoPduManager:
    def __init__(self, offset_path, pdudef_path):
        self.conv = PduBinaryConvertor(offset_path, pdudef_path)
    
    def append_pdu_def(self, service_config_path):
        self.conv.append_pdu_def(service_config_path)
        #dump for debug
        #print(json.dumps(self.conv.pdudef, indent=4))

    def get_pdu_from_name(self, robot_name, pdu_name):
        channel_id = self.conv.get_pdu_channel_id(robot_name, pdu_name)
        if channel_id is None:
            print(f"ERROR: can not find robo_name={robot_name} pdu_name={pdu_name}")
            return None
        return HakoPdu(self.conv, robot_name, channel_id)

    def get_pdu(self, robot_name, channel_id):
        return HakoPdu(self.conv, robot_name, channel_id)

def main():
    if len(sys.argv) != 7:
        print("Usage: pdu_io.py <offset_path> <pdudef.json> <robo_name> <channel_id> {r|w} <io_file>")
        sys.exit()

    offset_path = sys.argv[1]
    pdudef_path = sys.argv[2]
    robo_name = sys.argv[3]
    channel_id = (int)(sys.argv[4])
    io_type = sys.argv[5]
    io_file = sys.argv[6]

    obj = PduBinaryConvertor(offset_path, pdudef_path)

    if io_type == "w":
        with open(io_file, 'r') as file:
            json_data = json.load(file)
        
        binary_data = obj.json2bin(robo_name, channel_id, json_data)
        
        if binary_data is not None:
            with open('./binary.bin', 'wb') as bin_file:  # バイナリ書き込みモードで開く
                bin_file.write(binary_data)
            hex_data = binary_data.hex()
            print(hex_data)
        else:
            print("ERROR: Conversion failed or channel not found.")
    else:
        with open(io_file, 'rb') as file:
            binary_data = file.read()
        
        json_data = obj.bin2json(robo_name, channel_id, binary_data)
        
        if json_data is not None:
            print(json.dumps(json_data, indent=4))
        else:
            print("ERROR: Conversion failed or channel not found.")

if __name__ == "__main__":
    main()
