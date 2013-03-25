#!/usr/bin/env python

import time
from MatrixDisplay import MatrixDisplay

if __name__ == "__main__":
    disp = MatrixDisplay("/tmp/matrix_display")

    disp.scroll_disable()
    disp.time("Today is a %A")

    time.sleep(2)

    disp.scroll_left()

    for i in xrange(1000):
        disp.text("Counting, %d " % i)
        time.sleep(0.05)

