#!/usr/bin/python
# -*- coding: utf-8 -*-
import glob
import json
import sys
from hako_binary import offset_parser

class OffsetMap:
    def __init__(self, offset_path):
        self.off_path = offset_path
        self.map = {}

    def get(self, typename):
        if (typename in self.map) == False:
            filepath = self.find_filepath(self.off_path +"/*/", typename + ".offset")
            lines = offset_parser.parse_offset(filepath)
            self.map[typename] =  lines
        return self.map[typename]

    def find_filepath(self, path, filename):
        f_array = filename.split('/')
        if (len(f_array) > 1):
            filename = f_array[len(f_array) - 1]
        #print("path=" + path)
        #print("filename=" + filename)
        tmp = glob.glob(path + filename, recursive=True)
        if (len(tmp) == 0):
            print("ERROR: find_filepath(" + path + " ," + filename +")" )
            exit(1)
        return tmp[0]

def create_offmap(offset_path):
    return OffsetMap(offset_path)


