#!/usr/bin/python3

# A simple motor test script. Turns motors one by one for a short time.

import registers as regs
import sys
import time
import udp_command


def main(argv):
    if len(argv) < 2:
        print('usage: %s <IP address>' % argv[0])
        return
    udp = udp_command.UDPCommand(argv[1], 1234)
    udp.write_reg(regs.ENABLE, 1)
    udp.write_reg(regs.LIMITTER, 1000)
    udp.write_reg(regs.TEST_MODE, 0xa5)

    udp.write_reg(regs.OUT_M0, 100)
    time.sleep(1)
    udp.write_reg(regs.OUT_M0, 0)
    udp.write_reg(regs.OUT_M1, 100)
    time.sleep(1)
    udp.write_reg(regs.OUT_M1, 0)
    udp.write_reg(regs.OUT_M2, 100)
    time.sleep(1)
    udp.write_reg(regs.OUT_M2, 0)
    udp.write_reg(regs.OUT_M3, 100)
    time.sleep(1)
    udp.write_reg(regs.OUT_M3, 0)
    udp.write_reg(regs.TEST_MODE, 0)

    udp.close()


if __name__ == "__main__":
    main(sys.argv)
