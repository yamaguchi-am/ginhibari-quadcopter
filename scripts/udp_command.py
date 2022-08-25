#!/usr/bin/env python

import os
import socket
import struct
import sys
import registers as regs

if sys.version_info.major == 3:
    # Python 3
    _pack_byte = lambda b: b.to_bytes(1, 'little')
    _pack_short = lambda i: i.to_bytes(2, 'little')
else:
    # Python 2.7
    _pack_byte = lambda b: struct.pack('<B', b)
    _pack_short = lambda i: struct.pack('<H', i)


def _encode_int16(value):
    # todo: check range
    if value < 0:
        value += 65536
    return _pack_short(value)


def _decode_int16(d):
    v = (int(d[0]) | (int(d[1]) << 8))
    if v > 32767:
        v -= 65536
    return v


class UDPCommand:

    def __init__(self, addr, port):
        self.host = addr
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.settimeout(0.01)

    def read_reg(self, addr):
        cmd = b'\x00' + _pack_byte(addr)
        self.socket.sendto(cmd, (self.host, self.port))
        try:
            d = self.socket.recv(64)
        except socket.timeout:
            return None
        return _decode_int16(d[1:])

    def write_reg(self, addr, value):
        if value < 0:
            value += 65536
        cmd = b'\x01' + _pack_byte(addr) + _pack_short(value)
        self.socket.sendto(cmd, (self.host, self.port))

    def bulk_read(self, addr, n):
        cmd = b'\x02' + _pack_byte(addr)
        cmd += _pack_byte(n)
        self.socket.sendto(cmd, (self.host, self.port))
        result = []
        try:
            d = self.socket.recv(64)
        except socket.timeout:
            return None
        if d[0] != 0x02:
            return None
        assert int(d[1]) == addr
        assert int(d[2]) == n
        assert len(d) == 3 + n * 2
        for i in range(0, n):
            j = i * 2 + 3
            result.append(_decode_int16(d[j:j + 2]))
        return result

    def bulk_write(self, addr, values):
        cmd = b'\x03' + _pack_byte(addr)
        cmd += _pack_byte(len(values))
        for v in values:
            cmd += _encode_int16(v)
        self.socket.sendto(cmd, (self.host, self.port))

    def read_all_status(self):
        FIRST_STATUS_REG = regs.CALIBRATE
        rd = self.bulk_read(FIRST_STATUS_REG,
                            regs.N_REGISTERS - FIRST_STATUS_REG)
        if rd is None:
            return None, None
        d = [None] * FIRST_STATUS_REG + rd
        elapsed = 0
        for i in range(2):
            v = d[regs.ELAPSED_L + i]
            if v < 0:
                v += 65536
            elapsed |= (v << (i * 16))
        return elapsed, d

    def close(self):
        self.socket.close()


def main(argv):
    if len(argv) < 2:
        print('usage: %s <IP address>' % argv[0])
        return
    udp = UDPCommand(argv[1], 1234)
    while True:
        msg = input("> ")
        if not msg:
            break
        if msg:
            udp.write_reg(regs.GAIN_PITCH_P, int(msg))
            udp.write_reg(regs.JS_THROTTLE, 100)
        print(udp.read_all_status())

    udp.close()


if __name__ == '__main__':
    main(sys.argv)
