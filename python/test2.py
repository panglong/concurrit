__author__ = 'tayfun elmas'

from jsonservice import *
import unittest

class JsonServerTests(unittest.TestCase):

    # check if tobytes of json_socket works correctly
    def test_encode_decode(self):
        self.assertEqual(len(("%4d" % 123).encode()), 4)

        # generate the message in bytes
        myname = "Tayfun Elmas"
        msg = myname.encode()

        # generate the size in bytes
        sz = len(msg)
        sz = ("%4d" % sz).encode()
        # its size must be 4
        self.assertEqual(len(sz), 4)

        # try to recover the string from the encoded msg and encoded sz
        msg = msg[0 : int(sz.decode())].decode()

        self.assertEqual(msg, myname)


    def test_only_server(self):
        try:
            service = json_service("", 8080)
            service.start()

        except Exception as e:
            service.shutdown()
            print("Error!, closing the service!")
            raise e
        else:
            service.shutdown()

    def test_server_client(self):
        try:

            service = json_service("", 8080)
            service.start()

            client = json_client("127.0.0.1", 8080)
            client.connect()

            endpoint = service.accept()

            client.send(name="Tayfun", surname="Elmas")
            client.send({"isim": "Azize", "soyisim": "Elmas"})

            myname = endpoint.receive()
            self.assertEqual(myname["name"], "Tayfun")
            self.assertEqual(myname["surname"], "Elmas")

            hername = endpoint.receive()
            self.assertEqual(hername["isim"], "Azize")
            self.assertEqual(hername["soyisim"], "Elmas")

        except Exception as e:
            if service != None: service.shutdown()
            if client != None: client.close()
            if endpoint != None: endpoint.close()
            print("Error!, closing the service!")
            raise e
        else:
            if service != None: service.shutdown()
            if client != None: client.close()
            if endpoint != None: endpoint.close()
        

    ########################################################

if __name__ == '__main__':
    unittest.main()