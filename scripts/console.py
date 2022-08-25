#!/usr/bin/python3

import collections
import pygame

import filters
import flight
import panels
import registers as regs

WHITE = (255, 255, 255)
GRAY = (128, 128, 128)
GREEN = (0, 255, 0)
RED = (255, 0, 0)
DARK_GREEN = (0, 64, 0)

WINDOW_WIDTH = 640
WINDOW_HEIGHT = 480

# TODO: Share these conversions code or factors with firmwares.
# int toReg(float radians) { return radians * 1800 / M_PI; }
# float toRadians(int reg) { return reg * M_PI / 1800; }
# constexpr float kJoystickRollPitchGain = M_PI / 180 * 30.0 / 1024;


def jsToAngle(val):
    return val * 30.0 / 1024


def drawBarMeter(screen, frame, val):
    h = frame.height - 2
    w = frame.width - 2
    bar = pygame.Rect(frame.left + 1, frame.top + 1 + (h * (1 - val)), w,
                      h * val)
    pygame.draw.rect(screen, WHITE, frame, 1)
    pygame.draw.rect(screen, GREEN, bar)


def drawXYGraph(screen, frame, roll, pitch, tgt_roll, tgt_pitch):
    y0 = frame.top + frame.height / 2
    x0 = frame.left + frame.width / 2
    radius = frame.width / 2
    pygame.draw.circle(screen, DARK_GREEN, (x0, y0), radius)
    scale = frame.width / 180  # 1800 = 180 deg
    pygame.draw.line(screen, GRAY, (frame.left, y0), (frame.right, y0))
    pygame.draw.line(screen, GRAY, (x0, frame.top), (x0, frame.bottom))
    pygame.draw.circle(screen, GREEN, (x0 + roll * scale, y0 - pitch * scale),
                       1)
    pygame.draw.circle(screen, WHITE,
                       (x0 + tgt_roll * scale, y0 - tgt_pitch * scale), 1)


class XyGraph(panels.Panel):

    def __init__(self, left, top, width, height, roll, pitch, tgt_roll,
                 tgt_pitch):
        super().__init__(left, top, width, height)
        self.roll = roll
        self.pitch = pitch
        self.tgt_roll = tgt_roll
        self.tgt_pitch = tgt_pitch

    def addChild(self, child):
        assert False

    def draw(self, screen, rect):
        r = self.getRect()
        r.left += rect.left
        r.top += rect.top
        drawXYGraph(screen, r, self.roll, self.pitch, self.tgt_roll,
                    self.tgt_pitch)


def drawSignal(screen, frame, val):
    fr = pygame.Rect(frame.left, frame.top, frame.width, frame.height)
    y0 = frame.top + frame.height / 2
    x0 = frame.left + frame.width / 2
    radius = frame.width / 2
    pygame.draw.circle(screen, GREEN, (x0, y0), radius, 1)
    if val == 0:
        color = GREEN
    else:
        color = RED
    pygame.draw.circle(screen, color, (x0, y0), radius)


class Graph:

    def __init__(self, name, duration, min, max):
        self.data = collections.deque()
        self._duration = duration
        self._min = min
        self._max = max
        self._name = name

    def update(self, t, data):
        while self.data and self.data[0][0] < t - self._duration:
            self.data.popleft()
        self.data.append((t, data))

    def draw(self, screen, current_time, origin_x, origin_y):
        w = 200
        h = 100
        pts = []
        T_SCALE = w / self._duration
        rect = pygame.Rect(origin_x, origin_y, w, h)
        screen.fill(DARK_GREEN, rect)

        def get_y(v):
            return origin_y + h - ((v - self._min) * h /
                                   (self._max - self._min))

        for p in self.data:
            x = origin_x + w - (current_time - p[0]) * T_SCALE
            y = get_y(p[1])
            pts.append((x, y))
            pts.append((x, y))
        if pts:
            pygame.draw.lines(screen, WHITE, False, pts, 1)
        y0 = int(get_y(0))
        pygame.draw.line(screen, GRAY, (origin_x, y0), (origin_x + w, y0))
        font = pygame.font.Font(None, 24)
        screen.blit(font.render(self._name, True, WHITE), (origin_x, origin_y))


class GraphItem:

    def __init__(self, name, func, minval, maxval):
        self.name = name
        self.func = func
        self.min = minval
        self.max = maxval


class ConsoleData:

    def __init__(self):
        self.t = 0
        self.ctrl = flight.Control()
        self.regs = None
        self.conf = None

    def ready(self):
        return (self.ctrl is not None and self.regs is not None)


class Console:

    def __init__(self):
        self._TEXT_SIZE = 32
        self._TEXT_COLOR = (255, 255, 255)
        self.screen = pygame.display.set_mode((512, 512))
        self.DURATION = 10e3
        self._graph_items = [
            GraphItem('throttle', lambda r, ctrl: ctrl.throttle, 0, 1023),
            GraphItem('pitch', lambda r, ctrl: r[regs.PITCH_ANGLE], -1800,
                      1800),
            GraphItem('roll', lambda r, ctrl: r[regs.ROLL_ANGLE], -1800, 1800),
            GraphItem('yaw', lambda r, ctrl: r[regs.YAW_ANGLE], -1800, 1800),
            # GraphItem('yaw velocity', lambda r, ctrl : r[regs.ROTATION_Z], -180000, 180000),
        ]
        self._graphs = []
        for i in self._graph_items:
            self._graphs.append(Graph(i.name, self.DURATION, i.min, i.max))
        self._batt_filter = filters.MeanFilter(300)

    def update2(self, data):
        return self.update(data.t, data.ctrl, data.regs, data.conf)

    def update(self, t, ctrl, r, conf):
        XYGRAPH_FRAME = pygame.Rect(100, 10, 100, 100)
        CALIB_SIGNAL_FRAME = pygame.Rect(80, 0, 30, 30)

        self._batt_filter.push(r[regs.BATT_AD])

        self.screen.fill((0, 0, 0))
        font = pygame.font.Font(None, self._TEXT_SIZE)

        root_view = panels.Panel(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)
        list_view = panels.Panel(0, 100, 100, 100)
        root_view.addChild(list_view)
        ITEMS = [
            ('t', t),
            ('THROTTLE', ctrl.throttle),
            ('BATTERY', r[regs.BATT_VOLTAGE]),
        ]
        y = 0
        for i in ITEMS:
            label = i[0]
            val = i[1]
            text = '%s %04d' % (label, val)
            list_view.addChild(panels.Label(0, y, self._TEXT_SIZE, text,
                                            WHITE))
            y += self._TEXT_SIZE
        idx = 0
        x = 300
        y = 0
        for i in self._graph_items:
            self._graphs[idx].update(t, i.func(r, ctrl))
            self._graphs[idx].draw(self.screen, t, x, y)
            idx += 1
            y += 120

        METER_HEIGHT = 40
        outs = [
            (regs.OUT_M0, 0, METER_HEIGHT + 10),
            (regs.OUT_M1, 40, 0),
            (regs.OUT_M2, 40, METER_HEIGHT + 10),
            (regs.OUT_M3, 0, 0),
        ]
        meter_font = pygame.font.Font(None, 20)
        for x in outs:
            f = pygame.Rect(x[1], x[2], 10, METER_HEIGHT)
            drawBarMeter(self.screen, f, r[x[0]] / 1023)
            self.screen.blit(
                meter_font.render('%4d' % r[x[0]], True, self._TEXT_COLOR),
                (x[1], x[2] + METER_HEIGHT))

        rpView = XyGraph(XYGRAPH_FRAME.left, XYGRAPH_FRAME.top,
                         XYGRAPH_FRAME.width, XYGRAPH_FRAME.height,
                         r[regs.ROLL_ANGLE], r[regs.PITCH_ANGLE],
                         jsToAngle(ctrl.roll) * 10,
                         jsToAngle(ctrl.pitch) * 10)
        root_view.addChild(rpView)
        TRIM_POS = (0, 200)
        if conf is not None and conf.trim_x:
            self.screen.blit(
                meter_font.render('TRIM %4d %4d' % (conf.trim_x, conf.trim_y),
                                  True, self._TEXT_COLOR), TRIM_POS)
        drawSignal(self.screen, CALIB_SIGNAL_FRAME, r[regs.CALIBRATE])

        root_view.draw(self.screen, pygame.Rect(0, 0, 0, 0))
        pygame.display.update()
