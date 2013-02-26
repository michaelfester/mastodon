#!/usr/bin/env python
#
# Copyright 2012 8pen

"""Utility class for byte arrays"""

def to_int(byte_array, offset, chunk_size):
    value = 0
    for i in range(0, chunk_size):
        value += byte_array[offset + i] << (chunk_size-i-1)*8
    return value