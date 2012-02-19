__author__ = 'tayfun elmas'

from counit import coroutine
import unittest

class CoroutineTests(unittest.TestCase):

    ########################################################
    def test_format(self):
        self.assertEquals("%4d" % 12, "  12")
        self.assertEquals(int("%4d" % 12), 12)
        self.assertEquals(len("%5d" % 12), 5)
    ########################################################
    def test_split(self):
        self.assertListEqual("name=tayfun&surname=elmas".split("&"), ["name=tayfun", "surname=elmas"])

    ########################################################
    def test_star_args(self):

        def f(x, *args):
            if len(args) == 0:
                return "Empty"
            else:
                return str(args[x])

        self.assertEqual(f(1), "Empty")
        self.assertEqual(f(1, 42, 43, 44), "43")

    ########################################################
    def test_generator(self):
        def generator(k):
            for i in range(1,k):
                print(i)
                coroutine.transfer(None)

        c = coroutine(generator)(100)

        for i in range(1, 50):
            coroutine.transfer(c)
        
    ########################################################
    def test_positive_negative(self):

        def positive(k, neg):
            for i in range(1,k):
                print(i)
                coroutine.transfer(neg)

        def negative(k):
            for i in range(1,k):
                print(-i)
                coroutine.transfer(None)

        neg = coroutine(negative)
        pos = coroutine(positive)

        neg(100)
        pos(100, neg)

#        for i in range(1, 50): # this creates a deadlock since neg returns back to pos, not here!
        coroutine.transfer(pos)
            
    ########################################################
    def test_ping_pong(self):

        self.x = 10
        
        def f_ping(pong):
            while 1:
                print("ping" + str(self.x))
                self.x = self.x - 1
                coroutine.transfer(pong)
                if self.x == 0:
                    break


        def f_pong(ping):
            while 1:
                print("pong" + str(self.x))
                self.x = self.x - 1
                coroutine.transfer(ping)
                if self.x == 0:
                    break

        ping = coroutine(f_ping)
        pong = coroutine(f_pong)

        ping(pong)
        pong(ping)

        # returns back when both ping and pong terminates
        coroutine.transfer(ping)


    ########################################################

    def test_nested_coroutines(self):
        pass


    ########################################################

if __name__ == '__main__':
    unittest.main()