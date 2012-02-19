__author__ = 'tayfun elmas'

import unittest
from setservice import *
from concurrent_list import concurrent_list
from counit import coroutine


########################################################

class concurrent_dispatcher:
    def __init__(self):
        self.clients = []

    def add(self, client):
        self.clients.append(client.co_run)

    # default implementation runs all up to termination
    def run(self, testcase):
        for client in self.clients:
            client.finish()

########################################################

class list_client:
    def __init__(self, cl):
        self.cl = cl
        self.co_run = coroutine(self.do_run, "Client")(self.cl)

    def do_run(self, cl):
        self.run(cl)

    def ended(self):
        return self.co_run.ended

    def set_schedule(self, sc):
        self.co_run.schedule = sc

    # to override
    def run(self, cl):
        pass

########################################################

from expect import *

class ListTests(unittest.TestCase):

    def test_7(self):
        # create a list
        cl = concurrent_list(2)

        # create a client
        class client1(list_client):
            def run(self, cl):
                # this must be atomic
                cl.add(1)
                cl.remove(2)

        # create another client
        class client2(list_client):
            def run(self, cl):
                # .....
                pass


        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):
                c1 = self.clients[0]
                c2 = self.clients[1]

                expect_true(cl.is_empty())
                c1()
                expect_equals([1], cl.contents())
                c2()
                expect_true(cl.is_empty())
                c1()
                expect_equals([2], cl.contents())
                c2()
                expect_true(cl.is_empty())

                # call super
                concurrent_dispatcher.run(self, testcase)

                testcase.assertListEqual(cl.contents(), [])

        c1 = client1(cl)
        c2 = client2(cl)

        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)




    ########################################################

    def test_6(self):
        # create a list
        cl = concurrent_list()
        cl.capacity = 1

        # create a client
        class client1(list_client):
            def run(self, cl):

                cl.add(1)
                cl.add(2)

        # create another client
        class client2(list_client):
            def run(self, cl):

                taken = cl.take()
                expect_equals(1, taken)

                taken = cl.take()
                expect_equals(2, taken)


        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):
                c1 = self.clients[0]
                c2 = self.clients[1]

                expect_true(cl.is_empty())

                c1() # adds 1
                expect_equals([1], cl.contents())

                c2() # takes 1
                expect_true(cl.is_empty())

                c1() # add 2
                expect_equals([2], cl.contents())

                c2() # takes 2
                expect_true(cl.is_empty())

                # call super
                concurrent_dispatcher.run(self, testcase)

                testcase.assertListEqual(cl.contents(), [])

        c1 = client1(cl)
        c2 = client2(cl)

        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)


    ########################################################


    def test_5(self):
        # create a list
        cl = concurrent_list(1,1,2)

        # create a client
        class client1(list_client):
            def run(self, cl):
                cl.add(3)
                expect_equals(2, cl.remove_all(1))

        # create another client
        class client2(list_client):
            def run(self, cl):
                lst = [1, 2, 3]
                cl.foreach(expect_list(lst))


        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):
                c1 = self.clients[0]
                c2 = self.clients[1]

                c1() # add 3
                expect_equals([1,1,2,3], cl)

                c2() # read 1
                expect_equals([1,1,2,3], cl)

                c1.finish()
                expect_equals([2,3], cl)

                c2.finish()

                # call super
                concurrent_dispatcher.run(self, testcase)

                testcase.assertListEqual(cl.contents(), [2, 3])

        c1 = client1(cl)
        c2 = client2(cl)

        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)


    ########################################################


    def test_4(self):
        # expected sequence of states: [1] -> [1,2] -> [1]

        # create a list
        cl = concurrent_list(1)

        # create a client
        class client1(list_client):
            def run(self, cl):
                cl.add(2)
                expect_true(cl.remove(1))

        # create another client
        class client2(list_client):
            def run(self, cl):
                lst = [1, 2]
                cl.foreach(expect_list(lst))


        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):
                c1 = self.clients[0]
                c2 = self.clients[1]

                c1()
                c2()
                c1()

                # call super
                concurrent_dispatcher.run(self, testcase)

                testcase.assertListEqual(cl.contents(), [2])

        c1 = client1(cl)
        c2 = client2(cl)

        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)



    ########################################################

    def test_3_1(self):
        # expected sequence of states: [] -> [2] -> [2,1]


        # create a list
        cl = concurrent_list()

        # create a client
        class client1(list_client):
            def run(self, cl):
                self.set_schedule(schedule(True, False))
                cl.add(1)

        # create another client
        class client2(list_client):
            def run(self, cl):
                self.set_schedule(schedule(False, False))
                cl.add(2)

        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):

                # run all in order
                while 1:
                    if len(self.clients) == 0:
                        break
                    to_remove = []
                    for client in self.clients:
                        client()
                        if client.ended:
                            to_remove.append(client)
                    for client in to_remove:
                        self.clients.remove(client)

                testcase.assertListEqual(cl.contents(), [2,1])

        c1 = client1(cl)
        c2 = client2(cl)

        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)




    ########################################################

    def test_3(self):
        # create a list
        cl = concurrent_list()

        # create a client
        class client1(list_client):
            def run(self, cl):
                cl.add(1)

        # create another client
        class client2(list_client):
            def run(self, cl):
                cl.add(2)

        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):
                c1 = self.clients[0]
                c2 = self.clients[1]

                c2()
                c1()

                # call super
                concurrent_dispatcher.run(self, testcase)

                testcase.assertListEqual(cl.contents(), [2,1])

        c1 = client1(cl)
        c2 = client2(cl)

        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)




    ########################################################

    def test_2(self):
        # create a list
        cl = concurrent_list()

        # create a client
        class client1(list_client):
            def run(self, cl):
                cl.add(1)
                cl.add(2)
                cl.add(3)

        # create another client
        class client2(list_client):
            def run(self, cl):
                expect_equals([1], cl.contents())

        c1 = client1(cl)
        c2 = client2(cl)

        class dispatcher(concurrent_dispatcher):
            def run(self, testcase):
                c1 = self.clients[0]
                c2 = self.clients[1]

                c1()
                c2.finish()
                c1.finish()

                # call super
                concurrent_dispatcher.run(self, testcase)

                testcase.assertListEqual(cl.contents(), [1, 2, 3])


        dispatcher = dispatcher()
        dispatcher.add(c1)
        dispatcher.add(c2)

        dispatcher.run(self)


    ########################################################

if __name__ == '__main__':
    unittest.main()