import counit

__author__ = 'tayfun elmas'

import unittest
from concurrent_list import concurrent_list
from counit import coroutine, Assume, Assert, explore, Scenario

########################################################

class list_client(coroutine):
    def __init__(self, id):
        super(list_client, self).__init__(self.run, "client" + str(id))
        self.__call__()

    # to override
    def run(self):
        pass

########################################################

class ListTests(unittest.TestCase):

    def __test_3(self):
        # create a list
        cl = None

        # create a client
        class client1(list_client):
            def run(self):
                cl.add(1)

        # create another client
        class client2(list_client):
            def run(self):
                cl.add(2)

        c1 = client1(1)
        c2 = client2(2)

        def create_list():
            nonlocal cl
            cl = concurrent_list()

        def final_checks():
            Assume(cl.contents() == [1, 2])

        scenario = explore([create_list], [c1, c2], [final_checks], scenario="scenario1.txt", search_if_fails=True, save=False)

        print("\nPrinting the scenario")
        print(str(scenario))

    def __test_2(self):
        # create a list
        cl = None

        # create a client
        class client1(list_client):
            def run(self):
                cl.add(1)

        # create another client
        class client2(list_client):
            def run(self):
                cl.add(2)

        c1 = client1(1)
        c2 = client2(2)

        def create_list():
            nonlocal cl
            cl = concurrent_list()

        def final_checks():
            Assume(cl.contents() == [2, 1])

        scenario = explore([create_list], [c1, c2], [final_checks], scenario="scenario1.txt", search_if_fails=False, save=False)

        print("\nPrinting the scenario")
        print(str(scenario))

    ########################################################

    def test_1(self):
        # create a list
        cl = None

        # create a client
        class client1(list_client):
            def run(self):
                cl.add(1)

        # create another client
        class client2(list_client):
            def run(self):
                cl.add(2)

        c1 = client1(1)
        c2 = client2(2)

        def create_list():
            nonlocal cl
            cl = concurrent_list()

        def final_checks():
            Assume(cl.contents() == [2, 1])
            Assert(1 in cl)
            Assert(2 in cl)

        scenario = explore([create_list], [c1, c2], [final_checks], scenario=None)
        
        print("\nPrinting the scenario")
        print(str(scenario))

        scenario.save("scenario1.txt")

        print("\nLoaded scenario")
        print(Scenario.load("scenario1.txt"))




    ########################################################

if __name__ == '__main__':
    unittest.main()