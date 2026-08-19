#!/usr/bin/env python3

# Junk test server to verify curl calls

from http.server import BaseHTTPRequestHandler, HTTPServer

class Handler(BaseHTTPRequestHandler):

    def do_GET(self):
        body = b"Hello from the LoRa network!\n"

        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()

        self.wfile.write(body)

    def log_message(self, format, *args):
        print(format % args)


server = HTTPServer(("0.0.0.0", 8000), Handler)

print("Listening on 0.0.0.0:8000")

server.serve_forever()
