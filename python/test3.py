__author__ = 'tayfun elmas'

import unittest
from setservice import *

class SetServerTests(unittest.TestCase):

    ########################################################

    def test_contents_two_clients(self):
        try:

            HOST = "127.0.0.1"
            PORT = 65333

            service = set_service("", PORT)
            service.start()

            client1 = set_client(HOST, PORT, service)
            client1.connect()

            client2 = set_client(HOST, PORT, service)
            client2.connect()

            client1.add(41)

            contents1 = client1.contents()

            self.assertEqual(contents1, str([41]))

        except Exception as e:
            if service != None: service.shutdown()
            if client1 != None: client1.close()
            print("Error!, closing the service!")
            raise e
        else:
            if service != None: service.shutdown()
            if client1 != None: client1.close()

    ########################################################

    def test_add(self):
        try:

            service = set_service("", 65532)
            service.start()

            client = set_client("127.0.0.1", 65532, service)
            client.connect()

            client.add(41)
            client.add(42)
            client.add(43)

            contents = client.contents()

            self.assertEqual(contents, str([41, 42, 43]))
            self.assertNotEqual(contents, str([41, 42, 43, 44]))

        except Exception as e:
            if service != None: service.shutdown()
            if client != None: client.close()
            print("Error!, closing the service!")
            raise e
        else:
            if service != None: service.shutdown()
            if client != None: client.close()


    ########################################################

    def test_new(self):
        try:

            service = set_service("", 65532)
            service.start()

            client = set_client("127.0.0.1", 65532, service)
            client.connect()

            contents = client.contents()

            self.assertEqual(contents, str([]))
            self.assertNotEqual(contents, str([1]))

        except Exception as e:
            if service != None: service.shutdown()
            if client != None: client.close()
            print("Error!, closing the service!")
            raise e
        else:
            if service != None: service.shutdown()
            if client != None: client.close()

    ########################################################

    def __test_add(self):
        try:

            service = set_service("", 8080)
            service.start()

            client = set_client("127.0.0.1", 8080)
            client.connect()

            client.add("1")

            contents = client.contents()
            self.assertEqual(contents, "1")

        except Exception as e:
            if service != None: service.shutdown()
            if client != None: client.close()
            print("Error!, closing the service!")
            raise e
        else:
            if service != None: service.shutdown()
            if client != None: client.close()
            
    ########################################################
    
    def __test_set_service(self):
        try:

            service = set_service("", 8080)
            service.start()

            client = set_client("127.0.0.1", 8080)
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