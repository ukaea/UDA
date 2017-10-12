import unittest
import pyidam
from pyidam import Properties
import numpy as np


import sys
if sys.version_info < (2,7):
    def _assertIsInstance(self, val, inst):
        if isinstance(val, inst):
            return True
        raise AssertionError()
    unittest.TestCase.assertIsInstance = _assertIsInstance


class ClientTests(unittest.TestCase):

    __name__ = 'Client Tests'

    def test_getting_server_name(self):
        name = pyidam.Client.server
        self.assertEqual(name, "idam0")

    def test_getting_server_port(self):
        port = pyidam.Client.port
        self.assertEqual(port, 56565)

    def test_setting_server_name(self):
        pyidam.Client.server = "idam1"
        name = pyidam.Client.server
        self.assertEqual(name, "idam1")
        pyidam.Client.server = "idam0"

    def test_setting_server_port(self):
        pyidam.Client.port = 56566
        port = pyidam.Client.port
        self.assertEqual(port, 56566)
        pyidam.Client.port = 56565

    def test_setting_property_value(self):
        value = pyidam.Client.get_property(Properties.PROP_META)
        self.assertFalse(value)
        pyidam.Client.set_property(Properties.PROP_META, True)
        value = pyidam.Client.get_property(Properties.PROP_META)
        self.assertTrue(value)
        pyidam.Client.set_property(Properties.PROP_META, False)
        value = pyidam.Client.get_property(Properties.PROP_META)
        self.assertFalse(value)

    def test_exception_thrown_on_setting_properties_to_invalid_value(self):
        self.assertRaises(pyidam.IdamException, pyidam.Client.set_property, Properties.PROP_META, 3)

    def test_creating_new_client(self):
        client = pyidam.Client()
        self.assertIsInstance(client, pyidam.Client)

    def test_getting_signal_data(self):
        client = pyidam.Client()
        signal = client.get('ip', 18299)
        self.assertIsInstance(signal, pyidam.Signal)

    def test_getting_bad_signal_data_throws_exception(self):
        client = pyidam.Client()
        self.assertRaises(pyidam.IdamException, client.get, 'foo', -1)


class SignalTests(unittest.TestCase):

    __name__ = 'Signal Tests'

    def setUp(self):
        self._client = pyidam.Client()
        self._signal = self._client.get("ip", 18299)

    def test_signal_has_label(self):
        self.assertEqual(self._signal.label, "Plasma Current")

    def test_signal_has_units(self):
        self.assertEqual(self._signal.units, "kA")

    def test_signal_has_rank(self):
        self.assertEqual(self._signal.rank, 1)

    def test_signal_data_is_numpy_array(self):
        data = self._signal.data
        self.assertIsInstance(data, np.ndarray)

    def test_signal_data_len_of_shape_is_same_as_rank(self):
        data = self._signal.data
        self.assertEqual(len(data.shape), self._signal.rank)

    def test_signal_can_get_rank_number_of_dims(self):
        for i in xrange(0, self._signal.rank):
            self._signal.dim(i)


class DimTest(unittest.TestCase):

    __name__ = 'Dim Tests'

    def setUp(self):
        self._client = pyidam.Client()
        signal = self._client.get("ip", 18299)
        self._dim = signal.dim(0)

    def test_dim_has_label(self):
        self.assertEqual(self._dim.label, "Time (sec)")

    def test_dim_has_units(self):
        self.assertEqual(self._dim.units, "s")

    def test_dim_has_number(self):
        self.assertEqual(self._dim.number, 0)

    def test_dim_data_is_1D_numpy_array(self):
        data = self._dim.data
        self.assertIsInstance(data, np.ndarray)
        self.assertEqual(len(data.shape), 1)


class StructuredDataTests(unittest.TestCase):

    __name__ = 'Tree Signal Tests'

    def setUp(self):
        self._client = pyidam.Client()

    def test_get_structured_data(self):
        tree = self._client.get("meta::getdata(context=data, device=MAST, /lastshot)", "MAST::")
        self.assertIsInstance(tree, pyidam.StructuredData)

    def test_structured_data_has_name(self):
        tree = self._client.get("meta::getdata(context=data, device=MAST, /lastshot)", "MAST::")
        self.assertEqual(tree.name, 'ROOT')

    def test_structured_data_has_properties(self):
        tree = self._client.get("meta::getdata(context=data, device=MAST, /lastshot)", "MAST::")
        self.assertEqual(tree.count, 1)
        self.assertEqual(tree.rank, 1)
        self.assertEqual(tree.shape, 1)
        self.assertEqual(tree.type, 'DATALASTSHOT')

    def test_structured_data_has_children(self):
        tree = self._client.get("meta::getdata(context=data, device=MAST, /lastshot)", "MAST::")
        self.assertEqual(len(tree.children), 1)

    def test_child_node(self):
        tree = self._client.get("meta::getdata(context=data, device=MAST, /lastshot)", "MAST::")
        node = tree.children[0]
        self.assertEqual(len(node.children), 0)
        self.assertEqual(node.name, 'data')
        self.assertEqual(node.lastshot, 30473)

    def test_access_netcdf_file_as_structured_data(self):
        tree = self._client.get("/", "/home/dgm/IDAM/source/idl/putdata/meta/sxr/metaSxrSpecview.nc")
        self.assertIsInstance(tree, pyidam.StructuredData)

    def test_access_child_using_path_notation(self):
        tree = self._client.get("/", "/home/dgm/IDAM/source/idl/putdata/meta/sxr/metaSxrSpecview.nc")
        node = tree["data/sxr/cameraData"]
        self.assertEqual(node.name, "cameraData")

    def test_incorrect_path_raises_exception(self):
        tree = self._client.get("/", "/home/dgm/IDAM/source/idl/putdata/meta/sxr/metaSxrSpecview.nc")
        self.assertRaises(KeyError, lambda: tree["foo/bar"])

    def test_multiple_matching_children_raises_exception(self):
        tree = self._client.get("/", "/home/dgm/IDAM/source/idl/putdata/meta/sxr/metaSxrSpecview.nc")
        self.assertRaises(KeyError, lambda: tree["data/sxr/cameraData/data"])

    def test_access_one_of_multiple_children_using_at(self):
        tree = self._client.get("/", "/home/dgm/IDAM/source/idl/putdata/meta/sxr/metaSxrSpecview.nc")
        node = tree["data/sxr/cameraData"]
        self.assertTrue(len(node.children) > 1)
        node = tree["data/sxr/cameraData/data@0"]
        self.assertEqual(node.name, "data")

if __name__ == '__main__':
    unittest.main()
