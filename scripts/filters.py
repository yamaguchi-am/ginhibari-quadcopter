import collections


class MeanFilter:

    def __init__(self, size):
        self.data = collections.deque()
        self.size = size

    def push(self, x):
        self.data.append(x)
        if len(self.data) > self.size:
            self.data.popleft()

    def get_mean(self):
        l = len(self.data)
        if l < self.size:
            return None
        sum = 0
        for x in self.data:
            sum += x
        return sum / l
