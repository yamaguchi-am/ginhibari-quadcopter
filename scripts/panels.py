import pygame
from pygame.locals import *


class Panel:

    def __init__(self, left, top, width, height):
        self.rect = pygame.Rect(left, top, width, height)
        self.children = []

    def getRect(self):
        return self.rect

    def addChild(self, child):
        self.children.append(child)

    def draw(self, screen, rect):
        r = self.getRect()
        r.left += rect.left
        r.top += rect.top
        for n in self.children:
            n.draw(screen, r)


class Label(Panel):

    def __init__(self, left, top, size, str, color=(255, 255, 255)):
        width = 0
        height = 0
        super().__init__(left, top, width, height)
        self.str = str
        self.color = color
        self.size = size

    def addChild(self, child):
        assert (False)

    def draw(self, screen, rect):
        r = self.getRect()
        r.left += rect.left
        r.top += rect.top
        font = pygame.font.Font(None, self.size)
        origin_x = r.left
        origin_y = r.top
        screen.blit(font.render(self.str, True, self.color),
                    (origin_x, origin_y))


def main():
    pygame.init()
    screen = pygame.display.set_mode((512, 512))

    active = True
    i = 0
    while active:
        i += 1
        p1 = Panel(100, 50, 100, 10)
        l1 = Label(10, 10, 24, 'Hello')
        l2 = Label(10, 40, 24, 'World')
        l3 = Label(40, 54, 48, '%d' % i)
        p1.addChild(l1)
        p1.addChild(l2)
        p1.addChild(l3)
        for e in pygame.event.get():
            if e.type == pygame.locals.QUIT:
                active = False
            elif e.type == pygame.locals.KEYDOWN:
                c = e.key
                if c == ord('\x1b'):
                    active = False
        screen.fill((0, 0, 0))
        p1.draw(screen, pygame.Rect(0, 0, 512, 512))
        pygame.display.update()


if __name__ == "__main__":
    main()
