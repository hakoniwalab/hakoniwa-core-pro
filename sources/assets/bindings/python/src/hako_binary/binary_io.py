#!/usr/bin/python
# -*- coding: utf-8 -*-

import glob
import re
import os
from sys import byteorder
import struct

def binToint8(binary):
    return struct.unpack('b', binary)[0]

def binToint16(binary):
    return struct.unpack('h', binary)[0]

def binToint32(binary):
    return struct.unpack('i', binary)[0]

def binTobool(binary):
    return struct.unpack('i', binary)[0]

def binToint64(binary):
    return struct.unpack('q', binary)[0]

def binTouint8(binary):
    return struct.unpack('B', binary)[0]

def binTouint16(binary):
    return struct.unpack('H', binary)[0]

def binTouint32(binary):
    return struct.unpack('I', binary)[0]

def binTouint64(binary):
    return struct.unpack('Q', binary)[0]

def binTofloat32(binary):
    return struct.unpack('f', binary)[0]

def binTofloat64(binary):
    return struct.unpack('d', binary)[0]

def binTostring(binary, max_len=128):
    try:
        sub = binary[:max_len]
        end = sub.find(b'\0')
        if end == -1:
            end = max_len
        return sub[:end].decode('utf-8')
    except UnicodeDecodeError as e:
        print("UnicodeDecodeError:")
        print(f"  ERROR: {e}")
        print(f"  RAW DATA: {binary}")
        print(f"  HEXDUMP: {' '.join(f'{b:02x}' for b in binary)}")
        raise e



def int8Tobin(arg):
    return struct.pack('b', arg)

def int16Tobin(arg):
    return struct.pack('h', arg)

def int32Tobin(arg):
    return struct.pack('i', arg)

def boolTobin(arg):
    return struct.pack('i', arg)

def int64Tobin(arg):
    return struct.pack('q', arg)

def uint8Tobin(arg):
    return struct.pack('B', arg)

def uint16Tobin(arg):
    return struct.pack('H', arg)

def uint32Tobin(arg):
    return struct.pack('I', arg)

def uint64Tobin(arg):
    return struct.pack('Q', arg)

def float32Tobin(arg):
    return struct.pack('f', arg)

def float64Tobin(arg):
    return struct.pack('d', arg)

def stringTobin(arg):
    return arg.encode(encoding='utf-8') + b'\x00'

def typeTobin(type, arg):
    if (type == "int8"):
        return int8Tobin(arg)
    elif (type == "uint8"):
        return uint8Tobin(arg)
    elif (type == "int16"):
        return int16Tobin(arg)
    elif (type == "uint16"):
        return uint16Tobin(arg)
    elif (type == "int32"):
        return int32Tobin(arg)
    elif (type == "bool"):
        return boolTobin(arg)
    elif (type == "uint32"):
        return uint32Tobin(arg)
    elif (type == "int64"):
        return int64Tobin(arg)
    elif (type == "uint64"):
        return uint64Tobin(arg)
    elif (type == "float32"):
        return float32Tobin(arg)
    elif (type == "float64"):
        return float64Tobin(arg)
    elif (type == "string"):
        return stringTobin(arg)
    else:
        return None

def binTovalue(type, arg):
    if (type == "int8"):
        return binToint8(arg)
    elif (type == "uint8"):
        return binTouint8(arg)
    elif (type == "int16"):
        return binToint16(arg)
    elif (type == "uint16"):
        return binTouint16(arg)
    elif (type == "int32"):
        return binToint32(arg)
    elif (type == "bool"):
        return binTobool(arg)
    elif (type == "uint32"):
        return binTouint32(arg)
    elif (type == "int64"):
        return binToint64(arg)
    elif (type == "uint64"):
        return binTouint64(arg)
    elif (type == "float32"):
        return binTofloat32(arg)
    elif (type == "float64"):
        return binTofloat64(arg)
    elif (type == "string"):
        return binTostring(arg)
    else:
        return None
def binToArrayValues(type, arg):
    # little endian
    if (type == "int8"):
        return struct.unpack(f'<{len(arg)}b', arg)
    elif (type == "uint8"):
        return struct.unpack(f'<{len(arg)}B', arg)
    elif (type == "int16"):
        return struct.unpack(f'<{len(arg)//2}h', arg)
    elif (type == "uint16"):
        return struct.unpack(f'<{len(arg)//2}H', arg)
    elif (type == "int32"):
        return struct.unpack(f'<{len(arg)//4}i', arg)
    elif (type == "bool"):
        return struct.unpack(f'<{len(arg)//4}i', arg)
    elif (type == "uint32"):
        return struct.unpack(f'<{len(arg)//4}I', arg)
    elif (type == "int64"):
        return struct.unpack(f'<{len(arg)//8}q', arg)
    elif (type == "uint64"):
        return struct.unpack(f'<{len(arg)//8}Q', arg)
    elif (type == "float32"):
        return struct.unpack(f'<{len(arg)//4}f', arg)
    elif (type == "float64"):
        return struct.unpack(f'<{len(arg)//8}d', arg)
    else:
        return None

def typeTobin_array(type, values, elm_size=None):
    count = len(values)
    if type == "int8":
        return struct.pack(f'<{count}b', *values)
    elif type == "uint8":
        return struct.pack(f'<{count}B', *values)
    elif type == "int16":
        return struct.pack(f'<{count}h', *values)
    elif type == "uint16":
        return struct.pack(f'<{count}H', *values)
    elif type == "int32":
        return struct.pack(f'<{count}i', *values)
    elif type == "bool":
        return struct.pack(f'<{count}i', *values)
    elif type == "uint32":
        return struct.pack(f'<{count}I', *values)
    elif type == "int64":
        return struct.pack(f'<{count}q', *values)
    elif type == "uint64":
        return struct.pack(f'<{count}Q', *values)
    elif type == "float32":
        return struct.pack(f'<{count}f', *values)
    elif type == "float64":
        return struct.pack(f'<{count}d', *values)
    elif type == "string":
        if elm_size is None:
            raise ValueError("elm_size required for string array")
        binaries = []
        for val in values:
            raw = val.encode('utf-8') + b'\x00'
            if len(raw) > elm_size:
                raw = raw[:elm_size]
            buffer = bytearray(elm_size)
            buffer[:len(raw)] = raw
            binaries.append(buffer)
        return b''.join(binaries)
    else:
        raise ValueError(f"typeTobin_array: Unsupported type {type}")


def writeBinary(binary_data, off, bin):
    i = 0
    for data in bin:
        binary_data[off + i] = data
        i = i + 1

def readBinary(binary_data, off, size):
    return binary_data[off:off+size]

class PduMetaData:
    PDU_META_DATA_SIZE = 24
    PDU_META_DATA_MAGICNO = 0x12345678
    PDU_META_DATA_VERSION = 1
    def __init__(self):
        self.magicno = 0
        self.version = 0
        self.base_off = 0
        self.heap_off = 0
        self.total_size = 0
    def set_empty(self):
        self.magicno = PduMetaData.PDU_META_DATA_MAGICNO
        self.version = PduMetaData.PDU_META_DATA_VERSION
        self.base_off = PduMetaData.PDU_META_DATA_SIZE
    def to_bytes(self):
        data = bytearray()
        data.extend(self.magicno.to_bytes(4, byteorder='little'))
        data.extend(self.version.to_bytes(4, byteorder='little'))
        data.extend(self.base_off.to_bytes(4, byteorder='little'))
        data.extend(self.heap_off.to_bytes(4, byteorder='little'))
        data.extend(self.total_size.to_bytes(4, byteorder='little'))
        return data


class PduMetaDataParser:
    def __init__(self):
        self.meta = PduMetaData()

    def load_pdu_meta(self, binary_data):
        if len(binary_data) < PduMetaData.PDU_META_DATA_SIZE:
            return None
        magicno = binTovalue("uint32", readBinary(binary_data, 0, 4))
        version = binTovalue("uint32", readBinary(binary_data, 4, 4))
        if magicno != PduMetaData.PDU_META_DATA_MAGICNO or version != PduMetaData.PDU_META_DATA_VERSION:
            return None
        self.meta.magicno    = magicno
        self.meta.version    = version
        self.meta.base_off   = binTovalue("uint32", readBinary(binary_data, 8, 4))
        self.meta.heap_off   = binTovalue("uint32", readBinary(binary_data, 12, 4))
        self.meta.total_size = binTovalue("uint32", readBinary(binary_data, 16, 4))
        return self.meta
