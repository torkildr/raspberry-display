#!/usr/bin/env python

class RaspberryDisplay(object):
    def __init__(self, filename):
        self.filename = filename

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

    def clear(self):
        self.scroll_disable()
        self.text("")

    def __write(self, text):
        fifo = open(self.filename, 'w')
        fifo.write(text.decode('utf-8').encode('iso-8859-1'))
        fifo.close()

