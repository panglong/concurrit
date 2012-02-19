import inspect
import unittest
import sys

class Tests(unittest.TestCase):

    ########################################################

    def afunction(self):
        print("Line 1")
        print("Line 2")
        print("Line 3")
        return "Line 4"

    def test_inspect(self):
        lst = inspect.getsourcelines(self.afunction)
        for line in lst[0]:
            sys.stdout.write(line)

    ########################################################

    def test_none_string(self):
        self.assertEquals(str(None), "None")

if __name__ == '__main__':
    unittest.main()