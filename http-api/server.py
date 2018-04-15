#!/usr/bin/env python3

import json

from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse
from RaspberryDisplay import RaspberryDisplay

class DisplayHandler(BaseHTTPRequestHandler):
    display = RaspberryDisplay(debug=True)

    def _get_body(self):
        content_len = self.headers['content-length']
        if not content_len:
            return None

        return self.rfile.read(int(content_len))

    def _response(self, status_code):
        self.send_response(status_code)
        self.end_headers()

    def scroll(self, body):
        if not body or not 'arg' in body:
            self._response(400)
            return

        self.display.scroll(body['arg'])
        self._response(200)

    def text(self, body):
        if not body or not 'text' in body or not 'time' in body:
            self._response(400)
            return

        self.display.text(body['text'], showTime=body['time'])
        self._response(200)

    def brightness(self, body):
        if not body or not 'value' in body:
            self._response(400)
            return

        self.display.brightness(body['value'])
        self._response(200)

    def time(self, body):
        if not body or not 'format' in body:
            self._response(400)
            return

        self.display.time(body['format'])
        self._response(200)

    def clear(self, body):
        if body:
            self._response(400)
            return

        self.display.clear()
        self._response(200)

    def do_POST(self):
        handlers = {
            '/text': self.text,
            '/scroll': self.scroll,
            '/brightness': self.brightness,
            '/clear': self.clear,
            '/time': self.time,
        }

        if self.path in handlers:
            post_body = self._get_body()
            if post_body:
                body = json.loads(post_body.decode('utf-8'))
                handlers[self.path](body)
            else:
                handlers[self.path](None)
            return

        self._response(404)

if __name__ == '__main__':
    server = HTTPServer(('0.0.0.0', 8080), DisplayHandler)
    print('Starting server at 8080')

server.serve_forever()

