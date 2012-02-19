__author__ = 'elmas'

import json
import socket

########################################################################

class json_socket:
    def __init__(self, addr, socket = None):
        self.addr = addr
        self.socket = socket
        print("Initializing socket with address: ", addr)

    def send_object(self, obj = None, **args):
        if obj is None:
            obj = dict(args)
        msg = json.dumps(obj)
        self.send_message(msg)

    def receive_object(self):
        msg = self.receive_message()
        obj = json.loads(msg)
        return obj

    def tobytes(self, s):
        return s.encode()

    def tostring(self, b):
        return b.decode()

    def send_message(self, msg):
        msg = self.tobytes(msg)

        # send length
        sz = len(msg)
        sz = self.tobytes("%4d" % sz)
        assert len(sz) == 4
        self.do_send(sz)

        # send the actual message
        self.do_send(msg)

    def receive_message(self):
        # get length
        sz = int(self.tostring(self.do_receive(4)))

        # get the actual message
        msg = self.tostring(self.do_receive(sz))

        return msg

    def do_send(self, msg):
        totalsent = 0
        while totalsent < len(msg):
            sent = self.socket.send(msg[totalsent:])
            if sent == 0:
                raise RuntimeError("socket connection broken")
            totalsent = totalsent + sent
        return totalsent

    def do_receive(self, sz):
        msg = b''
        while len(msg) < sz:
            chunk = self.socket.recv(sz-len(msg))
            if chunk == b'':
                raise RuntimeError("socket connection broken")
            msg = msg + chunk
        return msg

    def close(self):
        self.socket.close()

########################################################################

class json_server_socket(json_socket):

    def __init__(self, host, port):
        self.addr = (host, port)

    # server mode
    def start(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(self.addr)
        self.socket.listen(1)

    def accept(self):
        (sock, addr) = self.socket.accept()
        return json_socket(addr, sock) # will be used to create a json_endpoint

########################################################################

class json_client_socket(json_socket):
    def __init__(self, host, port):
        json_socket.__init__(self, (host, port), socket.socket(socket.AF_INET, socket.SOCK_STREAM))

    def connect(self):
        self.socket.connect(self.addr)

########################################################################

class json_endpoint:

    def __init__(self, jsonsocket):
        self.socket = jsonsocket

    def send(self, obj = None, **args):
        self.socket.send_object(obj, **args)

    def receive(self):
        return self.socket.receive_object()

    def close(self):
        self.socket.close()

########################################################################

class json_service:

    def __init__(self, host, port):
        self.serversocket = json_server_socket(host, port)

    def start(self):
        self.serversocket.start()

    def accept(self):
        return json_endpoint(self.serversocket.accept())

    def shutdown(self):
        self.serversocket.close()


########################################################################

class json_client:
    
    def __init__(self, host, port):
        self.socket = json_client_socket(host, port)

    def connect(self):
        self.socket.connect()

    def send(self, obj = None, **args):
        self.socket.send_object(obj, **args)

    def receive(self):
        return self.socket.receive_object()

    def close(self):
        self.socket.close()

    