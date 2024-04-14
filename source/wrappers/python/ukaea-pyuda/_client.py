from __future__ import (division, print_function, absolute_import)

import cpyuda

from ._signal import Signal
from ._string import String
from ._structured import StructuredData
from ._video import Video
from ._tree import Tree
from ._version import __version__

from six import with_metaclass
import logging
from collections import namedtuple
from collections.abc import Iterable
import sys
try:
    from enum import Enum
except ImportError:
    Enum = object


class ClientMeta(type):
    """
    Metaclass used to add class level properties
    """
    def __init__(cls, what, bases=None, dict=None):
        type.__init__(cls, what, bases, dict)

    @property
    def port(cls):
        return cpyuda.get_server_port()

    @port.setter
    def port(cls, value):
        cpyuda.set_server_port(value)

    @property
    def server(cls):
        return cpyuda.get_server_host_name()

    @server.setter
    def server(cls, value):
        cpyuda.set_server_host_name(value)


class Client(with_metaclass(ClientMeta, object)):
    """
    A class representing the UDA client.

    This is a pythonic wrapper around the low level c_uda.Client class which contains the wrapped C++ calls to
    UDA.
    """
    __metaclass__ = ClientMeta

    def __init__(self, debug_level=logging.ERROR):
        self.version = __version__
        assert self.version == cpyuda.get_build_version().decode(), "mismatching pyuda and c-library versions"

        logging.basicConfig(level=debug_level)
        self.logger = logging.getLogger(__name__)

        self._registered_subclients = {}

        try:
            from mast.geom import GeomClient
            from mast import MastClient
            self._registered_subclients['geometry'] = GeomClient(self)
            self._registered_subclients['geometry_signal_mapping'] = GeomClient(self)
            self._registered_subclients['get_images'] = MastClient(self)
            self._registered_subclients['get_shot_date_time'] = MastClient(self)
            self._registered_subclients['latest_shot'] = MastClient(self)
            self._registered_subclients['latest_source_pass'] = MastClient(self)
            self._registered_subclients['latest_source_pass_in_range'] = MastClient(self)            
            self._registered_subclients['listGeomSignals'] = GeomClient(self)
            self._registered_subclients['listGeomGroups'] = GeomClient(self)
            self._registered_subclients['list'] = MastClient(self)
            self._registered_subclients['list_archive_files'] = MastClient(self)
            self._registered_subclients['list_archive_file_info'] = MastClient(self)
            self._registered_subclients['list_archive_directories'] = MastClient(self)
            self._registered_subclients['list_file_signals'] = MastClient(self)            
            self._registered_subclients['list_signals'] = MastClient(self)
            self._registered_subclients['put'] = MastClient(self)
            self._registered_subclients['list_shots'] = MastClient(self)
        except ImportError:
            pass

    def get_file(self, source_file, output_file=None):
        """
        Retrieve file using bytes plugin and write to file
        :param source_file: the full path to the file
        :param output_file: the name of the output file
        :return:
        """

        result = cpyuda.get_data("bytes::read(path=%s)" % source_file, "")

        with open(output_file, 'wb') as f_out:
            result.data().tofile(f_out)

        return

    def get_text(self, source_file):
        """
        Retrive a text file using the bytes plugin and decode as string
        :param source_file: the full path to the file
        :return:
        """

        result = cpyuda.get_data("bytes::read(path=%s)" % source_file, "")

        if sys.version_info[0] <= 2:
            result_str = result.data().tostring()
        else:
            result_str = result.data().tobytes().decode('utf-8')
        return result_str

    @classmethod
    def _unpack(cls, result, time_first, time_last):
        if result.error_code() != 0:
            if result.error_message():
                raise cpyuda.ServerException(result.error_message().decode())
            else:
                raise cpyuda.ServerException("Unknown server error")

        if result.is_tree():
            tree = result.tree()
            if tree.data()['type'] == 'VIDEO':
                return Video(StructuredData(tree))
            else:
                return StructuredData(tree.children()[0])
        elif result.is_string() and result.rank() <= 1:
            return String(result)
        elif result.is_capnp():
            return Tree(result.capnp_tree())

        signal = Signal(result)

        if time_first:
            signal.set_time_first()
        elif time_last:
            signal.set_time_last()

        return signal

    def get_batch(self, signals, sources, time_first=False, time_last=False, **kwargs):
        if not isinstance(signals, Iterable) or isinstance(signals, str):
            raise ValueError("first argument must be a non-string iterable collection")
        if isinstance(sources, str):
            sources = [sources] * len(signals)
        else:
            try:
                sources = [str(s) for s in sources]
            except TypeError:
                sources = [str(sources)] * len(signals)
        if len(signals) != len(sources):
            raise ValueError("arguments must have the same number of elements")
        results = cpyuda.get_data_batch(signals, sources)
        signals = []
        for result in results:
            signals.append(self._unpack(result, time_first, time_last))
        return signals

    def get(self, signal, source="", time_first=False, time_last=False, **kwargs):
        """
        UDA get data method.

        :param signal: the name of the signal to get
        :param source: the source of the signal
        :param kwargs: additional optional keywords for geometry data
        :return: a subclass of pyuda.Data
        """
        # Standard signal
        result = cpyuda.get_data(str(signal), str(source))
        return self._unpack(result, time_first, time_last)

    def __getattr__(self, item):
        if item in self._registered_subclients:
            return getattr(self._registered_subclients[item], item)

        raise AttributeError("%s method not found in registered subclients" % item)

    @classmethod
    def get_property(cls, prop):
        return cpyuda.get_property(prop)

    @classmethod
    def set_property(cls, prop, value=None):
        cpyuda.set_property(prop, value)

    def close_connection(self):
        cpyuda.close_connection()

    def reset_connection(self):
        cpyuda.close_connection()
