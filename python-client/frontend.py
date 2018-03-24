#!/usr/bin/env python
# coding=utf-8

import time
from RaspberryDisplay import RaspberryDisplay

if __name__ == "__main__":
    disp = RaspberryDisplay("/tmp/raspberry-display")

    disp.scroll_disable()
    disp.time("Today is a %A")

    time.sleep(2)

    disp.scroll_disable()

    for i in xrange(100):
        disp.text("Counting, %d " % i)
        time.sleep(0.05)

    time.sleep(2)

    disp.scroll_auto()

    disp.text("This shouldn't scroll", True)

    time.sleep(4)

    disp.text("This is quite a long text, and should scroll...", True)

