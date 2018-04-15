#!/usr/bin/env python3
# coding=utf-8

import time
from RaspberryDisplay import RaspberryDisplay

if __name__ == "__main__":
    disp = RaspberryDisplay(debug=True)

    disp.scroll("none")
    disp.time("Today is a %A")

    time.sleep(2)

    for i in range(16):
        disp.brightness(i)
        disp.text("Brightness: %d" % i)
        time.sleep(0.1)
    
    time.sleep(2)

    for i in range(100):
        disp.text("Counting, %d " % i)
        time.sleep(0.05)

    time.sleep(2)

    disp.scroll("auto")

    disp.text("This shouldn't scroll", True)

    time.sleep(4)

    disp.text("This is quite a long text, and should scroll...", True)

