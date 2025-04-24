#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import sys
import base64

from hako_binary import binary_io
from hako_binary import offset_parser

def decode_base64(data):
    return base64.b64decode(data)

def binary_read(offmap, typename, binary_data):
    json_data = {}
    meta_parser = binary_io.PduMetaDataParser()
    meta = meta_parser.load_pdu_meta(binary_data)
    if meta is None:
        meta = binary_io.PduMetaData()
        meta.set_empty()
        #print("binary_data: size", len(binary_data))
        #print("meta.to_bytes(): size", len(meta.to_bytes()))
        binary_io.writeBinary(binary_data, 0, meta.to_bytes())
    binary_read_recursive(meta, offmap, binary_data, json_data, binary_io.PduMetaData.PDU_META_DATA_SIZE, typename)
    return json_data

def binary_read_recursive(meta: binary_io.PduMetaData, offmap, binary_data, json_data, base_off, typename):
    #lines = offmap[typename]
    lines = offmap.get(typename)
    for line in lines:
        off = offset_parser.member_off(line) + base_off
        type = offset_parser.member_type(line)
        name = offset_parser.member_name(line)
        size = offset_parser.member_size(line)
        #print(f"binary_read_recursive: {name} off={off} size={size} type={type}")
        if (offset_parser.is_primitive(line)):
            if (offset_parser.is_single(line)):
                bin = binary_io.readBinary(binary_data, off, size)
                #print(f"binary_read_recursive: {name} off={off} size={size} type={type} bin={bin}")
                value = binary_io.binTovalue(type, bin)
                json_data[name] = value
            elif (offset_parser.is_array(line)):
                array_value = binary_io.readBinary(binary_data, off, size)
                json_data[name + '__raw' ] = array_value
                json_data[name] = binary_io.binToArrayValues(type, array_value)
            else: #varray
                array_size = binary_io.binTovalue("int32", binary_io.readBinary(binary_data, off, 4))
                offset_from_heap = binary_io.binTovalue("int32", binary_io.readBinary(binary_data, off + 4, 4))
                one_elm_size = size
                #print(f"binary_read_recursive: {name} off={off} size={size} type={type} array_size={array_size} offset_from_heap={offset_from_heap}")
                array_value = binary_io.readBinary(binary_data, meta.heap_off + offset_from_heap, one_elm_size * array_size)
                json_data[name + '__raw' ] = array_value
                json_data[name] = binary_io.binToArrayValues(type, array_value)
        else:
            if (offset_parser.is_single(line)):
                tmp_json_data = {}
                binary_read_recursive(meta, offmap, binary_data, tmp_json_data, off, type)
                json_data[name] = tmp_json_data
            elif (offset_parser.is_array(line)):
                i = 0
                array_size = offset_parser.array_size(line)
                one_elm_size = int(size / array_size)
                array_value = []
                while i < array_size:
                    tmp_json_data = {}
                    binary_read_recursive(meta, offmap, binary_data, tmp_json_data, off + (i * one_elm_size), type)
                    array_value.append(tmp_json_data)
                    i = i + 1
                json_data[name] = array_value
            else: #varray
                array_size = binary_io.binTovalue("int32", binary_io.readBinary(binary_data, off, 4))
                offset_from_heap = binary_io.binTovalue("int32", binary_io.readBinary(binary_data, off + 4, 4))
                one_elm_size = size
                i = 0
                array_value = []
                while i < array_size:
                    tmp_json_data = {}
                    binary_read_recursive(meta, offmap, binary_data, tmp_json_data, meta.heap_off + offset_from_heap + (i * one_elm_size), type)
                    array_value.append(tmp_json_data)
                    i = i + 1
                json_data[name] = array_value


