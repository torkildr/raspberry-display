#!/usr/bin/env python

import sys

class RaspberryDisplay(object):
    def __init__(self, filename, debug=False):
        self.filename = filename
        self.debug = debug

        self.__open_pipe()
    
    def __open_pipe(self):
        self.fifo = open(self.filename, 'w')

    def __close_pipe(self):
        self.fifo.close()

    def scroll_left(self):
        self.__write("scroll-left:")

    def scroll_right(self):
        self.__write("scroll-right:")

    def scroll_reset(self):
        self.__write("scroll-left:")

    def scroll_auto(self):
        self.__write("scroll-auto:")

    def scroll_disable(self):
        self.__write("scroll-none:")

    def text(self, text, showTime = False):
        command = "text-time" if showTime else "text"
        self.__write("%s:%s" % (command, text))

    def time(self, format):
        self.__write("time:%s" % format)

    def brightness(self, brightness):
        if not brightness >= 0 and brightness <= 15:
            raise Exception("Brightness must be 0-15")
        self.__write("brightness:%d" % brightness)

    def clear(self):
        self.scroll_disable()
        self.text("")

    def __write(self, text):
        payload = text.decode('utf-8').encode('iso-8859-1') + "\n"

        if self.debug:
            sys.stdout.write(payload)

        self.fifo.write(payload)
        self.fifo.flush()

