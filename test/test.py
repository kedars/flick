#!/usr/bin/python

# Utility for testing the web server. Test cases:
# Assume the device supports 'n' simultaneous open sockets
#
# HTTP Server Tests
#
# 0. Firmware Settings:
# - Create a dormant thread whose sole job is to call httpd_stop() when instructed
# - Measure the following before httpd_start() is called:
#     - current free memory
#     - current free sockets
# - Measure the same whenever httpd_stop is called
# - Register maximum possible URI handlers: should be successful
# - Register one more URI handler: should fail
# - Deregister on URI handler: should be successful
# - Register on more URI handler: should succeed
# - Register separate handlers for /hello, /hello/type_html. Also
#   ensure that /hello/type_html is registered BEFORE  /hello. (tests
#   that largest matching URI is picked properly)
# - Create URI handler /adder. Make sure it uses a custom free_ctx
#   structure to free it up

# 1. Using Standard Python HTTP Client
# - simple GET on /hello (returns Hello World. Ensures that basic
#   firmware tests are complete, or returns error)
# - POST on /hello (should fail)
# - PUT on /hello (should fail)
# - simple POST on /echo (returns whatever the POST data)
# - simple PUT on /echo (returns whatever the PUT data)
# - GET on /echo (should fail)
# - simple GET on /hello/type_html (returns Content type as text/html)
# - simple GET on /hello/status_500 (returns HTTP status 500)
# - simple GET on /false_uri (returns HTTP status 404)
# - largest matching URI handler is picked is already verified because
#   of /hello and /hello/type_html tests
#
#
# 2. Session Tests
# - Sessions + Pipelining basics:
#    - Create max supported sessions 
#    - On session i, 
#          - send 3 back-to-back POST requests with data i on /adder
#          - read back 3 responses. They should be i, 2i and 3i
#    - Tests that 
#          - pipelining works
#          - per-session context is maintained for all supported
#            sessions
#    - Close all sessions
#
# - Cleanup leftover data: Tests that the web server properly cleans
#   up leftover data
#    - Create a session
#    - POST on /leftover_data with 52 bytes of data (data includes
#      \r\n)(the handler only
#      reads first 10 bytes and returns them, leaving the rest of the
#      bytes unread)
#    - GET on /hello (should return 'Hello World')
#    - POST on /false_uri with 52  bytes of data (data includes \r\n)
#      (should return HTTP 404) 
#    - GET on /hello (should return 'Hello World')



############# TODO TESTS #############
#
# - Timeout test
#    - Create a session and only Send 'GE' on the same (simulates a
#      client that left the network halfway through a request)
#    - Wait for recv-wait-timeout
#    - Server should automatically close the socket
#
# - Spillover test
#    - Create max supported sessions with the web server
#    - GET /hello on all the sessions (should return Hello World)
#    - Create one more session, this should fail
#    - GET /hello on all the sessions (should return Hello World)

# 3. Stress Tests
#
# - httperf
#     - Run the following httperf command:
#     httperf --server=10.31.130.126 --wsess=8,50,0.5 --rate 8#--burst-length 2
#
#     - The above implies that the test suite will open
#           - 8 simultaneous connections with the server
#           - the rate of opening the sessions will be 8 per sec. So in our
#             case, a new connection will be opened every 0.2 seconds for 1 second
#           - The burst length 2 indicates that 2 requests will be sent
#             simultaneously on the same connection in a single go
#           - 0.5 seconds is the time between sending out 2 bursts
#           - 50 is the total number of requests that will be sent out
#
#     - So in the above example, the test suite will open 8
#       connections, each separated by 0.2 seconds. On each connection
#       it will send 2 requests in a single burst. The bursts on a
#       single connection will be separated by 0.5 seconds. A total of
#       25 bursts (25 x 2 = 50) will be sent
#       out[kedars@Kedars-MacBook-Pro ezlite-v2]$

# 4. Leak Tests
# - Simple Leak test
#    - Simple GET on /hello/restart (returns success, stop web server, measures leaks, restarts webserver)
#    - Simple GET on /hello/restart_results (returns the leak results)
# - Leak test with open sockets
#    - Open 8 sessions
#    - Simple GET on /hello/restart (returns success, stop web server,
#      measures leaks, restarts webserver)
#    - All sockets should get closed
#    - Simple GET on /hello/restart_results (returns the leak results)


import threading
import socket
import time
import argparse
import requests
import sys

class Session:
    def __init__(self, addr, port):
        self.client = socket.create_connection((addr, port))
        self.target = addr
#        self.client.setblocking(0);
    def send_get(self, path):
        request = "GET " + path + " HTTP/1.1\r\nHost: " + self.target + "\r\n\r\n"
        self.client.send(request);
    def send_post(self, path, data):
        request = "POST " + path +  " HTTP/1.1\r\nHost: " + self.target + "\r\nContent-Length: " + str(len(data)) +"\r\n\r\n"
        self.client.send(request)
        self.client.send(data)
    def read_resp_hdr(self):
        state = 'nothing'
        resp_read = ''
        while True:
            char = self.client.recv(1)
            if char == '\r' and state == 'nothing':
                state = 'first_cr'
            elif char == '\n' and state == 'first_cr':
                state = 'first_lf'
            elif char == '\r' and state == 'first_lf':
                state = 'second_cr'
            elif char == '\n' and state == 'second_cr':
                state = 'second_lf'
            else:
                state = 'nothing'
            resp_read += char
            if state == 'second_lf':
               break; 
        # Handle first line
#        print resp_read
        line_hdrs = resp_read.splitlines()
        line_comp = line_hdrs[0].split()
        self.status = line_comp[1]
        del line_hdrs[0]
        self.encoding = ''
        self.content_type = ''
        # Process other headers
        for h in range(len(line_hdrs)):
            line_comp = line_hdrs[h].split(':')
            if line_comp[0] == 'Content-Length':
                self.content_len = int(line_comp[1])
            if line_comp[0] == 'Content-Type':
                self.content_type = line_comp[1]
            if line_comp[0] == 'Transfer-Encoding':
                self.encoding = line_comp[1]
    def read_resp_data(self):
        read_data = ''
        while len(read_data) != self.content_len:
            read_data += self.client.recv(self.content_len)
        self.content_len = 0
        return read_data
    def close(self):
        self.client.close()

def test_val(text, expected, received):
    if expected != received:
        print " Fail!"
        print "  [reason] " + text + ":"
        print "        expected: " + str(expected)
        print "        received: " + str(received)
        return False
    return True

class myThread (threading.Thread):
    def __init__(self, id, dut):
        threading.Thread.__init__(self)
        self.id = id
        self.dut = dut
        self.session = Session(dut, 80)

    def run(self):
        self.response = []

        # Pipeline 3 requests
        print "   Thread: Using adder start " + str(self.id)
        self.session.send_post('/adder', str(self.id));
        time.sleep(1)
        self.session.send_post('/adder', str(self.id));
        time.sleep(1)
        self.session.send_post('/adder', str(self.id));
        time.sleep(1)

        self.session.read_resp_hdr()
        self.response.append(self.session.read_resp_data())
        self.session.read_resp_hdr()
        self.response.append(self.session.read_resp_data())
        self.session.read_resp_hdr()
        self.response.append(self.session.read_resp_data())

    def adder_result(self):
        for i in range(len(self.response)):
#            print self.response[i]
            if not test_val("thread" + str(self.id) + ": response[" + str(i) + "]",
                            str(self.id * (i + 1)), str(self.response[i])):
                return False
        return True

    def close(self):
        self.session.close()

def get_hello():
    # GET /hello should return 'Hello World!'
    print "[test] GET /hello returns 'Hello World!' =>",
    r = requests.get("http://" + dut + "/hello")
    if not test_val("status_code", 200, r.status_code):
        return
    if not test_val("data", "Hello World!", r.text):
        return
    if not test_val("data", "application/json", r.headers['Content-Type']):
        return
    print "Success"

def post_hello():
    # PUT /hello returns 404'
    print "[test] PUT /hello returns 404' =>",
    r = requests.put("http://" + dut + "/hello", data="Hello")
    if not test_val("status_code", 404, r.status_code):
        return
    print "Success"

def put_hello():
    # POST /hello returns 404'
    print "[test] POST /hello returns 404' =>",
    r = requests.post("http://" + dut + "/hello", data="Hello")
    if not test_val("status_code", 404, r.status_code):
        return
    print "Success"

def post_echo():
    # POST /echo echoes data'
    print "[test] POST /echo echoes data' =>",
    r = requests.post("http://" + dut + "/echo", data="Hello")
    if not test_val("status_code", 200, r.status_code):
        return
    if not test_val("data", "Hello", r.text):
        return
    print "Success"

def put_echo():
    # POST /echo echoes data'
    print "[test] PUT /echo echoes data' =>",
    r = requests.put("http://" + dut + "/echo", data="Hello")
    if not test_val("status_code", 200, r.status_code):
        return
    if not test_val("data", "Hello", r.text):
        return
    print "Success"

def get_echo():
    # GET /echo returns 404'
    print "[test] GET /echo returns 404' =>",
    r = requests.get("http://" + dut + "/echo")
    if not test_val("status_code", 404, r.status_code):
        return
    print "Success"

def get_hello_type():
    # GET /hello/type_html returns text/html as Content-Type'
    print "[test] GET /hello/type_html has Content-Type of text/html =>",
    r = requests.get("http://" + dut + "/hello/type_html")
    if not test_val("status_code", 200, r.status_code):
        return
    if not test_val("data", "Hello World!", r.text):
        return
    if not test_val("data", "text/html", r.headers['Content-Type']):
        return
    print "Success"

def get_hello_status():
    # GET /hello/status_500 returns status 500'
    print "[test] GET /hello/status_500 returns status 500 =>",
    r = requests.get("http://" + dut + "/hello/status_500")
    if not test_val("status_code", 500, r.status_code):
        return
    print "Success"

def get_false_uri():
    # GET /false_uri returns status 404'
    print "[test] GET /false_uri returns status 404 =>",
    r = requests.get("http://" + dut + "/false_uri")
    if not test_val("status_code", 404, r.status_code):
        return
    print "Success"


def parallel_sessions_adder():
    # POSTs on /adder in parallel sessions
    print "[test] POST {pipelined} on /adder in " + str(max_sessions) + " sessions =>"
    t = []
# Create all sessions
    for i in xrange(max_sessions):
        t.append(myThread(i * 2, dut))

    for i in xrange(len(t)):
        t[i].start()

    for i in xrange(len(t)):
        t[i].join()

    for i in xrange(len(t)):
        if not t[i].adder_result():
            test_val("Thread" + str(i) + "Failed", "True", "False")
        t[i].close()
    print "Success"

def leftover_data_test():
    # Leftover data in POST is purged (valid and invalid URIs)
    print "[test] Leftover data in POST is purged (valid and invalid URIs) =>",
    s = Session(dut, 80)

    s.send_post('/leftover_data',
                "abcdefghijklmnopqrstuvwxyz\r\nabcdefghijklmnopqrstuvwxyz")
    s.read_resp_hdr()
    if not test_val("Partial data", "abcdefghij", s.read_resp_data()):
        return

    s.send_get('/hello')
    s.read_resp_hdr()
    if not test_val("Hello World Data", "Hello World!", s.read_resp_data()):
        return

    s.send_post('/false_uri',
                "abcdefghijklmnopqrstuvwxyz\r\nabcdefghijklmnopqrstuvwxyz")
    s.read_resp_hdr()
    if not test_val("False URI Status", str(404), str(s.status)):
        return
    s.read_resp_data()

    s.send_get('/hello')
    s.read_resp_hdr()
    if not test_val("Hello World Data", "Hello World!", s.read_resp_data()):
        return

    s.close()
    print "Success"

def spillover_session(max):
    # Session max_sessions + 1 is rejected
    print "[test] Session max_sessions + 1 is rejected =>",
    s = []
    for i in xrange(max + 1):
        print "Executing " + str(i)
        a = Session(dut, 80)
        a.send_get('/hello')
        s.append(a)
#        time.sleep(3)

#    if not test_val("status_code", 404, r.status_code):
#        return
    print "Success"

########### Execution begins here...
# Configuration
# Max number of threads/sessions
max_sessions = 8

parser = argparse.ArgumentParser(description='Run HTTPd Test')
parser.add_argument('-4','--ipv4', help='IPv4 address') # required=True)
parser.add_argument('-6','--ipv6', help='IPv6 address')
args = vars(parser.parse_args())

dut4 = args['ipv4']
dut6 = args['ipv6']

dut = dut4
print "### Basic HTTP Client Tests"
get_hello()
post_hello()
put_hello()
post_echo()
get_echo()
put_echo()
get_hello_type()
get_hello_status()
get_false_uri()
print "### Sessions and Context Tests"
parallel_sessions_adder()
leftover_data_test()
# XXX spillover_session(max_sessions)

sys.exit()

