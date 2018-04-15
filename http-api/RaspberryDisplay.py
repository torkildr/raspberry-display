import os
import stat
import signal
import sys
import time

class Timeout():
    class Timeout(Exception):
        pass

    def __init__(self, sec):
        self.sec = sec

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.raise_timeout)
        signal.alarm(self.sec)

    def __exit__(self, *args):
        signal.alarm(0)

    def raise_timeout(self, *args):
        raise Timeout.Timeout()

class RaspberryDisplay(object):
    def __init__(self, filename="/tmp/raspberry-display", debug=False, timeout=0.5):
        self.filename = filename
        self.debug = debug
        self.timeout = timeout
        self.fifo = None

        self._pipe_open()

    def _debug(self, text):
        if self.debug:
            sys.stdout.write(text)
            sys.stdout.flush()

    def _pipe_open(self):
        self._debug("Connecting to pipe")
        if self.fifo:
            try:
                self.fifo.close()
            except:
                pass

        while True:
            try:
                with Timeout(1):
                    self.fifo = open(self.filename, 'w', encoding='iso-8859-1')
                    self._debug('\n')
                    break
            except:
                pass
            self._debug(".")
            time.sleep(self.timeout)
    
    def _write_pipe(self, text):
        self._debug("sending \"%s\"\n" % text.strip())
        while True:
            try:
                self.fifo.write(text)
                self.fifo.flush()
                break
            except IOError:
                self._pipe_open()
                time.sleep(self.timeout)

    def _write(self, text):
        payload = "{}\n".format(text)
        self._write_pipe(payload)

    def scroll(self, value):
        self._write("scroll:%s" % value)
    
    def text(self, text, showTime = False):
        command = "text-time" if showTime else "text"
        self._write("%s:%s" % (command, text))

    def time(self, format):
        self._write("time:%s" % format)

    def brightness(self, brightness):
        if not brightness >= 0 and brightness <= 15:
            raise Exception("Brightness must be 0-15")
        self._write("brightness:%d" % brightness)

    def clear(self):
        self.scroll('disable')
        self.text("")

