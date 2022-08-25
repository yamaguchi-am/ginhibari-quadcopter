import config
import registers as regs
import time
import udp_command

STABILIZATION_TIME = 1000
MAX_CTRL_VAL = 1023


class Control:
    """Control command sent to a quadcopter."""

    def __init__(self):
        self.throttle = 0
        self.roll = 0
        self.pitch = 0
        self.yaw = 0
        self.buttons = 0


M_MANUAL = 0
M_ON_FLIGHT = 1
M_LANDING = 2
LANDING_TIME_MS = 1800
TAKEOFF_IDLE_DURATION = 1.0
TAKEOFF_RISE_DURATION = 0.5
TAKEOFF_TOTAL_TIME = TAKEOFF_IDLE_DURATION + TAKEOFF_RISE_DURATION


class Flight:
    """Flight controller state and tasks."""

    def __init__(self, cfg_name, addr, port):
        self.config = config.Config(cfg_name)
        self.comm = udp_command.UDPCommand(addr, port)
        self.init(self.config)
        self.set_trim(self.config.roll_trim, self.config.pitch_trim)
        self.trim_throttle = self.config.throttle_trim
        self.trim_x = self.config.roll_trim
        self.trim_y = self.config.pitch_trim
        self.mode = M_MANUAL
        self.land_start = 0
        self.takeoff_start_time = None

        self.last_yaw_angle = 0
        self._stable_since = None
        self.stabilized = False
        self.last_control = Control()
        self._batt_voltage = 4.2

    def __del__(self):
        self.comm.close()

    def set_trim(self, x, y):
        self.trim_x = x
        self.trim_y = y
        self.comm.write_reg(regs.TRIM_ROLL, self.trim_x)
        self.comm.write_reg(regs.TRIM_PITCH, self.trim_y)

    def set_throttle_trim(self, throttle):
        self.trim_throttle = throttle

    def add_trim(self, x, y):
        if x:
            self.trim_x += x
            self.comm.write_reg(regs.TRIM_ROLL, self.trim_x)
        if y:
            self.trim_y += y
            self.comm.write_reg(regs.TRIM_PITCH, self.trim_y)

    def take_off(self):
        self.mode = M_ON_FLIGHT
        self.takeoff_start_time = time.time()

    def is_on_flight(self):
        return self.mode == M_ON_FLIGHT or self.mode == M_LANDING

    def land(self):
        if self.mode == M_LANDING:
            return
        self.land_start = time.time() * 1000
        self.mode = M_LANDING

    def is_landed(self):
        return self.last_control.throttle == 0 and self.mode == M_MANUAL

    def add_trim_throttle(self, d):
        self.trim_throttle += d

    def task(self, st):
        # TODO: move logic to firmware.
        current = time.time() * 1000
        if st is not None and not self.stabilized:
            if st['yaw_angle'] == self.last_yaw_angle:
                if self._stable_since is None:
                    self._stable_since = current
                else:
                    if current > self._stable_since + STABILIZATION_TIME:
                        self.stabilized = True
            else:
                self._stable_since = current
                self.last_yaw_angle = st['yaw_angle']

        if self.mode == M_LANDING:
            if current > self.land_start + LANDING_TIME_MS:
                self.mode = M_MANUAL
        self._batt_voltage = st['filtered_batt_voltage'] * 1e-3

    def takeoff_throttle_bias(self):
        if self.takeoff_start_time is None:
            return 0
        elapsed = time.time() - self.takeoff_start_time
        if elapsed < TAKEOFF_IDLE_DURATION:
            return -100
        if elapsed < TAKEOFF_IDLE_DURATION + TAKEOFF_RISE_DURATION:
            return 110
        return 0

    def is_taking_off(self, current_time):
        if self.takeoff_start_time is None:
            return False
        elapsed = current_time - self.takeoff_start_time
        return elapsed < TAKEOFF_TOTAL_TIME

    def send(self, control):
        # TODO: move this to task() instead of requiring users to call send()
        self.last_control = control
        if self.mode == M_LANDING:
            t = self.trim_throttle - self.config.descend
        if self.mode == M_ON_FLIGHT:
            t = self.trim_throttle + control.throttle + self.takeoff_throttle_bias(
            )
        if self.mode == M_MANUAL:
            t = control.throttle

        if self._batt_voltage > 0:
            # TODO: publish modified throtthe?
            # Individual motor's PWM would already reflect this.
            t *= max(1.0, min(1.4, 4.2 / self._batt_voltage))
        t = max(0, min(MAX_CTRL_VAL, t))
        data = [
            int(t),
            int(control.yaw),
            int(control.pitch),
            int(control.roll)
        ]
        self.comm.bulk_write(regs.JS_THROTTLE, data)

    def read_all_status(self):
        return self.comm.read_all_status()

    def set_target_yaw(self, yaw):
        self.comm.write_reg(regs.TARGET_YAW, yaw)

    def set_target_yaw_to_current(self):
        self.comm.write_reg(regs.TARGET_YAW, self.last_yaw_angle)

    def calibrate(self):
        self.comm.write_reg(regs.CALIBRATE, 1)

    def enable(self, enable):
        if enable:
            self.comm.write_reg(regs.ENABLE, 1)
        else:
            self.comm.write_reg(regs.ENABLE, 0)

    def init(self, cfg):
        self.comm.write_reg(regs.CALIBRATE, 1)
        self.comm.write_reg(regs.ENABLE, 1)
        self.comm.write_reg(regs.LIMITTER, 1000)
        self.comm.write_reg(regs.HOVERING_VOLTAGE, 450)
        self.comm.write_reg(regs.DESCEND_TIME, 4000)

        self.comm.write_reg(regs.GAIN_PITCH_P, cfg.pitch_p)
        self.comm.write_reg(regs.GAIN_PITCH_D, cfg.pitch_d)
        self.comm.write_reg(regs.GAIN_ROLL_P, cfg.roll_p)
        self.comm.write_reg(regs.GAIN_ROLL_D, cfg.roll_d)
        self.comm.write_reg(regs.GAIN_YAW_P, cfg.yaw_p)
        self.comm.write_reg(regs.GAIN_YAW_D, cfg.yaw_d)
        self.comm.write_reg(regs.LOW_BATTERY_THRESHOLD,
                            cfg.low_battery_threshold)
