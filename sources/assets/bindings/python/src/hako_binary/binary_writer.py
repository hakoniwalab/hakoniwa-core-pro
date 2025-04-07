#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import sys

from hako_binary import binary_io
from hako_binary import offset_parser

class DynamicAllocator:
    def __init__(self, is_heap: bool):
        self.data = bytearray()
        self.offset_map = {}
        self.is_heap = is_heap

    def add(self, bytes_data, expected_offset=None, key=None):
        if self.is_heap == False:
            if expected_offset is not None:
                current_size = len(self.data)
                if current_size < expected_offset:
                    padding = bytearray(expected_offset - current_size)
                    self.data.extend(padding)
        
        offset = len(self.data)
        self.data.extend(bytes_data)
        
        if key:
            self.offset_map[key] = offset
        
        return offset

    def to_array(self):
        return self.data

    def size(self):
        return len(self.data)

    def get_offset(self, key):
        return self.offset_map.get(key, None)

class BinaryWriterContainer:
    def __init__(self):
        self.heap_allocator = DynamicAllocator(True)
        self.meta = binary_io.PduMetaData()
        self.meta.set_empty()

def binary_write(offmap, binary_data, json_data, typename):
    base_allocator = DynamicAllocator(False)
    bw_container = BinaryWriterContainer()
    binary_write_recursive(0, bw_container, offmap, base_allocator, json_data, typename)

    # メタデータの設定
    total_size = base_allocator.size() + bw_container.heap_allocator.size() + binary_io.PduMetaData.PDU_META_DATA_SIZE
    bw_container.meta.total_size = total_size
    bw_container.meta.heap_off = binary_io.PduMetaData.PDU_META_DATA_SIZE + base_allocator.size()

    # binary_data のサイズを total_size に調整
    if len(binary_data) < total_size:
        binary_data.extend(bytearray(total_size - len(binary_data)))
    elif len(binary_data) > total_size:
        del binary_data[total_size:]

    # メタデータをバッファにコピー
    binary_io.writeBinary(binary_data, 0, bw_container.meta.to_bytes())

    # 基本データをバッファにコピー
    binary_io.writeBinary(binary_data, bw_container.meta.base_off, base_allocator.to_array())

    # ヒープデータをバッファにコピー
    binary_io.writeBinary(binary_data, bw_container.meta.heap_off, bw_container.heap_allocator.to_array())

def get_binary(type, bin, elm_size):
    if type == "string":
        buffer = bytearray(elm_size)
        buffer[:len(bin)] = bin
        return buffer
    else:
        return bin

def binary_write_recursive(parent_off: int, bw_container: BinaryWriterContainer, offmap, allocator, json_data, typename):
    lines = offmap.get(typename)
    for key in json_data:
        line = offset_parser.select_by_name(lines, key)
        if line is None:
            continue
        type = offset_parser.member_type(line)
        off = offset_parser.member_off(line)
        if offset_parser.is_primitive(line):
            if offset_parser.is_single(line):
                bin = binary_io.typeTobin(type, json_data[key])
                bin = get_binary(type, bin, offset_parser.member_size(line))
                # print(f"{type} {key} = {json_data[key]} : bin: {bin} size: {offset_parser.member_size(line)} bin_size: {len(bin)}")
                allocator.add(bin, expected_offset=parent_off + off)
            elif offset_parser.is_array(line):
                elm_size = offset_parser.member_size(line)
                array_size = offset_parser.array_size(line)
                one_elm_size = int(elm_size / array_size)
                for i, elm in enumerate(json_data[key]):
                    bin = binary_io.typeTobin(type, elm)
                    bin = get_binary(type, bin, one_elm_size)
                    allocator.add(bin, expected_offset=(parent_off + off + i * one_elm_size))
            else:  # varray
                for i, elm in enumerate(json_data[key]):
                    bin = binary_io.typeTobin(type, elm)
                    bin = get_binary(type, bin, offset_parser.member_size(line))
                    bw_container.heap_allocator.add(bin, expected_offset=(off + i * offset_parser.member_size(line)))
        else:
            if offset_parser.is_single(line):
                binary_write_recursive(parent_off + off, bw_container, offmap, allocator, json_data[key], type)
            elif offset_parser.is_array(line):
                for i, elm in enumerate(json_data[key]):
                    elm_size = offset_parser.member_size(line)
                    array_size = offset_parser.array_size(line)
                    one_elm_size = int(elm_size / array_size)
                    binary_write_recursive((parent_off + off + i * one_elm_size), bw_container, offmap, allocator, elm, type)
            else:  # varray
                for i, elm in enumerate(json_data[key]):
                    binary_write_recursive(0, bw_container, offmap, bw_container.heap_allocator, elm, type)
