#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import sys
import hakopy
from hako_binary import offset_map
from hako_binary import binary_writer
from hako_binary import binary_reader
from hako_binary import binary_io

class PduBinaryConvertor:
    def __init__(self, offset_path, pdudef_path):
        self.offmap = offset_map.create_offmap(offset_path)
        self.pdudef = self._load_json(pdudef_path)

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

    def write(self):
        data = self.conv.json2bin(self.robot_name, self.channel_id, self.obj)
        return hakopy.pdu_write(self.robot_name, self.channel_id, data, len(data))

    def read(self):
        data = hakopy.pdu_read(self.robot_name, self.channel_id, self.pdu_size)
        if data == None:
            print('ERROR: hako_asset_pdu_read')
            return None
        self.obj = self.conv.bin2json(self.robot_name, self.channel_id, data)
        return self.obj

class HakoPduManager:
    def __init__(self, offset_path, pdudef_path):
        self.conv = PduBinaryConvertor(offset_path, pdudef_path)

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
