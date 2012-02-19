
from jsonservice import *
from counit import *

class set_service(json_service):

    def __init__(self, host, port):
        # call super
        json_service.__init__(self, host, port)
        self.id_to_set = {}
        self.nextid = 0
        self.co_handlers = []

    def start(self):
        self.co_start = coroutine(self.do_start, "Main handler")()()

    def cont(self, handler_id):
        if handler_id is None:
            # awake the main handler
            coroutine.transfer(self.co_start)
        else:
            # awake the client handler
            id = int(handler_id)
            assert 0 <= id < len(self.co_handlers)
            coroutine.transfer(self.co_handlers[id])

#    # does not finish
#    def do_start(self):
#        # call super
#        json_service.start(self)
#
#        coroutine.transfer(None) # yield
#
#        # start handling the requests
#        while 1:
#            endpoint = self.accept()
#            while 1:
#                request = endpoint.receive()
#                self.handle_request(endpoint, request)
#
#                coroutine.transfer(None) # yield

    # does not finish
    def do_start(self):
        # call super
        json_service.start(self)

        # start handling the requests
        while 1:
            coroutine.transfer(None) # yield
            
            endpoint = self.accept()
            # create a new continuation
            handler_id = len(self.co_handlers)
            co_handler = coroutine(self.do_handle_client, "Client handler " + str(handler_id))(endpoint, handler_id)()
            self.co_handlers.append(co_handler)

            coroutine.transfer()


    def do_handle_client(self, endpoint, handler_id):
        endpoint.send(handler_id=handler_id) # send the client id
        print("Sent client id:", handler_id)

        while 1:
            coroutine.transfer(None) # yield

            request = endpoint.receive()
            self.handle_request(endpoint, request)


    def handle_request(self, endpoint, request):

        print("Handling request:", request["op"])

        if request["op"] == "new":
            set = {}
            id = self.nextid;
            self.nextid += 1;
            self.id_to_set[id] = set

            endpoint.send(id=str(id))
            return

        if request["op"] == "add":
            id = int(request["id"])
            elt = int(request["elt"])
            set = self.id_to_set[id]
            assert set != None and isinstance(set, dict)
            set[elt] = True
            return

        if request["op"] == "contents":
            id = int(request["id"])
            set = self.id_to_set[id]
            assert set != None and isinstance(set, dict)
            contents = list(set.keys())
            endpoint.send(contents=str(contents))
            return
            
            
        

class set_client(json_client):

    def __init__(self, host, port, service):
        # call super
        json_client.__init__(self, host, port)
        self.service = service

    def connect(self):
        # call super
        json_client.connect(self)

        #awake the service
        self.service.cont(None)

        res = self.receive()
        self.handler_id = res["handler_id"]
        print("Received client id: ", self.handler_id)

        self.new()

    def new(self):
        self.send(op="new")

        #awake the service
        self.service.cont(self.handler_id)

        res = self.receive()
        self.set_id = res["id"]

    def add(self, elt):
        self.send(op="add", id=self.set_id, elt=elt)

        #awake the service
        self.service.cont(self.handler_id)

    def contents(self):
        self.send(op="contents", id=self.set_id)

        #awake the service
        self.service.cont(self.handler_id)
        
        res = self.receive()
        return res["contents"]
        
  