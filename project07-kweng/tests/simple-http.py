#!/usr/bin/env python3
# Reflects the requests from HTTP methods GET, POST, PUT, and DELETE
# Written by Nathan Hamiel (2010)

import time
import _thread as thread
from http.server import HTTPServer, BaseHTTPRequestHandler
from optparse import OptionParser

class SlowRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        time.sleep(10000)


class FastRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()

def main():
    def slow():
        slow = HTTPServer(('compute-cpu2.cms.caltech.edu', 8080), SlowRequestHandler)
        slow.serve_forever()

    def fast():
        fast = HTTPServer(('compute-cpu2.cms.caltech.edu', 8081), FastRequestHandler)
        fast.serve_forever()

    t1 = thread.start_new_thread(slow, ())
    fast()


        
if __name__ == "__main__":
    parser = OptionParser()
    parser.usage = ("Creates an http-server that will echo out any GET or POST parameters\n"
                    "Run:\n\n"
                    "   simple-http.py")
    (options, args) = parser.parse_args()
    
    main()
