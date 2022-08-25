#!/usr/bin/python3

# Non-ROS version of the manual control console using USB game controller.

import datetime
import math
import pygame
from pygame.locals import *
import sys

import console
import registers as regs
import flight


class Recorder:

    def __init__(self):
        self.active = False

    def start(self):
        current = datetime.datetime.now()
        self.path = '%s.txt' % current.isoformat(timespec='seconds')
        self.file = open(self.path, 'w')
        self.active = True

    def write(self, data):
        assert (self.active)
        self.file.write("\t".join([str(i[1]) for i in data]))
        self.file.write("\n")

    def stop(self):
        self.active = False
        self.file.close()


def main(argv):
    ctrl = flight.Control()
    recorder = Recorder()

    def onUpdate(joystick):

        def jsToAngle(val):
            # 1/10 degrees
            return val * 30.0 * 10

        THROTTLE_DEADBAND = 0.05
        if math.fabs(joystick.get_axis(1)) < THROTTLE_DEADBAND:
            ctrl.throttle = 0
        else:
            ctrl.throttle = max(0, -joystick.get_axis(1) * 1000)
        # TODO: update rudder handling
        # rudder = -joystick.get_axis(0)
        ctrl.yaw = 0
        ctrl.pitch = jsToAngle(-joystick.get_axis(4))
        ctrl.roll = jsToAngle(joystick.get_axis(3))

    def sync(c):
        t, d = f.read_all_status()
        if t is None:
            return
        # THROTTLE is not received. Fill sent value instead for printing.
        d[regs.JS_THROTTLE] = ctrl.throttle
        raw_items = [
            (regs.ROLL_ANGLE, 'angle roll'),
            (regs.PITCH_ANGLE, 'angle pitch'),
            (regs.YAW_ANGLE, 'angle yaw'),
            (regs.TARGET_ROLL, 'tatget roll'),
            (regs.TARGET_PITCH, 'tatget pitch'),
            (regs.TARGET_YAW, 'tatget yaw'),
            (regs.OUT_ROLL, 'out roll'),
            (regs.OUT_PITCH, 'out pitch'),
            (regs.OUT_YAW, 'out yaw'),
            (regs.JS_THROTTLE, 'throttle'),
            (regs.BATT_VOLTAGE, 'battery [mV]'),
            (regs.BATT_VOLTAGE_FILTERED, '  (after LPF)'),
            (regs.ROTATION_X, 'rot.x'),
            (regs.ROTATION_Y, 'rot.y'),
            (regs.ROTATION_Z, 'rot.z'),
            (regs.ACCEL_X, 'acc.x'),
            (regs.ACCEL_Y, 'acc.y'),
            (regs.ACCEL_Z, 'acc.z'),
            (regs.BATT_AD, 'batt.AD'),
            (regs.CTRL_INTERVAL, 'control interval'),
        ]
        items = [('time', t)]
        for i in raw_items:
            items.append((i[1], '%d' % d[i[0]]))
        print("\033[2J\033[%d;%dH" % (0, 0))
        for i in items:
            print("%-20s %s" % (i[0], i[1]))
        if recorder.active:
            recorder.write(items)
        con.update(t, ctrl, d, f)
        return {
            'yaw_angle': d[regs.YAW_ANGLE],
            'filtered_batt_voltage': d[regs.BATT_VOLTAGE_FILTERED],
        }

    pygame.joystick.init()

    try:
        joystick = pygame.joystick.Joystick(0)
        joystick.init()
        print(joystick.get_name())
    except pygame.error:
        print('joystick not found')
        # return

    pygame.init()
    con = console.Console()

    if len(argv) < 3:
        print('usage: %s <config file name> <IP address> [<port number>]' % argv[0])
        return
    if len(argv) > 3:
        port = int(argv[3])
    else:
        port = 1234
    f = flight.Flight(argv[1], argv[2], port)

    active = True
    while active:
        for e in pygame.event.get():
            if e.type == QUIT:
                active = False

            if e.type == pygame.locals.JOYAXISMOTION:
                onUpdate(joystick)
            elif e.type == pygame.locals.JOYBUTTONDOWN:
                pass
            elif e.type == pygame.locals.JOYBUTTONUP:
                pass
            elif e.type == pygame.locals.KEYDOWN:
                c = e.key
                f.calibrate()
                if c == ord('a'):
                    f.add_trim(-2, 0)
                if c == ord('d'):
                    f.add_trim(2, 0)
                if c == ord('w'):
                    f.add_trim(0, -2)
                if c == ord('x'):
                    f.add_trim(0, 2)
                if c == ord('0'):
                    f.enable(False)
                if c == ord('1'):
                    f.enable(True)
                if c == ord('r'):
                    recorder.start()
                if c == ord('e'):
                    recorder.stop()
                if c == ord('\x1b'):
                    active = False

        f.send(ctrl)
        st = sync(ctrl)
        if st is None:
            print("data not received")
            continue
        f.task(st)
        if ctrl.throttle == 0:
            f.set_target_yaw_to_current()
        if not f.stabilized:
            print("Initializing...")


if __name__ == "__main__":
    main(sys.argv)
