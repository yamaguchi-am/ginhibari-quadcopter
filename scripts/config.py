import configparser


class Config:

    def __init__(self, name=''):
        if not name:
            self.roll_p = 0
            self.roll_d = 0
            self.pitch_p = 0
            self.pitch_d = 0
            self.yaw_p = 0
            self.yaw_d = 0
            self.roll_trim = 0
            self.pitch_trim = 0
            self.throttle_trim = 0
            self.low_battery_threshold = 0
        else:
            load(name, self)


def load(name, cfg):
    c = configparser.ConfigParser()
    if not c.read(name):
        raise FileNotFoundError(name)
    f = c['feedback_gain']
    cfg.roll_p = int(f['RollP'])
    cfg.roll_d = int(f['RollD'])
    cfg.pitch_p = int(f['PitchP'])
    cfg.pitch_d = int(f['PitchD'])
    cfg.yaw_p = int(f['YawP'])
    cfg.yaw_d = int(f['YawD'])
    t = c['trim']
    cfg.roll_trim = int(t['Roll'])
    cfg.pitch_trim = int(t['Pitch'])
    cfg.throttle_trim = int(t['Throttle'])
    cfg.descend = int(t['Descend'])
    others = c['others']
    cfg.low_battery_threshold = int(others['LowBatteryThreshold'])


def main():
    c = Config('matrix1.ini')


if __name__ == "__main__":
    main()
