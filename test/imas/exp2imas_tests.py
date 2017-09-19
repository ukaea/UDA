import pyuda
import unittest


class TestStringMethods(unittest.TestCase):

    def setUp(self):
        self.client = pyuda.Client()

    def tearDown(self):
        self.client = None

    def test_x(self):
        result = self.client.get('EXP2IMAS::read(element=magnetics/flux_loop/Shape_of, shot=90000)', '')


if __name__ == '__main__':
    unittest.main()