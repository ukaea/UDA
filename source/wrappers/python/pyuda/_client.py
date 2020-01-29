from __future__ import (division, print_function, absolute_import)

import cpyuda

from ._signal import Signal
from ._string import String
from ._structured import StructuredData
from ._video import Video

from six import with_metaclass
import logging
from collections import namedtuple
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


class ListType(Enum):
    SIGNALS = 1
    SOURCES = 2
    SHOTS = 3


class Client(with_metaclass(ClientMeta, object)):
    """
    A class representing the IDAM client.

    This is a pythonic wrapper around the low level c_uda.Client class which contains the wrapped C++ calls to
    UDA.
    """
    __metaclass__ = ClientMeta

    def __init__(self, debug_level=logging.ERROR):
        logging.basicConfig(level=debug_level)
        self.logger = logging.getLogger(__name__)

        self._registered_subclients = {}

        try:
            from mastgeom import GeomClient
            self._registered_subclients['geometry'] = GeomClient(self)
            self._registered_subclients['listGeomSignals'] = GeomClient(self)
            self._registered_subclients['listGeomGroups'] = GeomClient(self)
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


    def get(self, signal, source, **kwargs):
        """
        IDAM get data method.

        :param signal: the name of the signal to get
        :param source: the source of the signal
        :param kwargs: additional optional keywords for geometry data
        :return: a subclass of pyuda.Data
        """
        # Standard signal
        result = cpyuda.get_data(str(signal), str(source))

        if result.is_tree():
            tree = result.tree()
            if tree.data()['type'] == 'VIDEO':
                return Video(StructuredData(tree))
            else:
                return StructuredData(tree.children()[0])
        elif result.is_string():
            return String(result)
        return Signal(result)

    def list(self, list_type, shot=None, alias=None, signal_type=None, signal_search=None, description_search=None):
        """
        Query the server for available data.

        :param list_type: the type of data to list, must be one of pyuda.ListType
        :param shot: the shot number, or None to return for all shots
        :param alias: the device alias, or None to return for all devices
        :param signal_type: the signal types {A|R\M|I}, or None to return for all types
        :param signal_search: string to filter on signal names. Use % as wildcard.
        :param description_search: string to filter on signal descriptions. Use % as wildcard.
        :return: A list of namedtuples containing the query data
        """
        if list_type == ListType.SIGNALS:
            list_arg = ""
        elif list_type == ListType.SOURCES:
            list_arg = "/listSources"
        else:
            raise ValueError("unknown list_type: " + str(list_type))

        args = ""
        if shot is not None:
            args += "shot=%s, " % str(shot)
        if alias is not None:
            args += "alias=%s, " % alias
        if signal_type is not None:
            if signal_type not in ("A", "R", "M", "I"):
                raise ValueError("unknown signal_type " + signal_type)
            args += "type=%s, " % signal_type
        if signal_search is not None:
            args += "signal_match=%s, " % signal_search
        if description_search is not None:
            args += "description=%s, " % description_search

        args += list_arg

        result = cpyuda.get_data("meta::list(context=data, cast=column, %s)" % args, "")
        if not result.is_tree():
            raise RuntimeError("UDA list data failed")

        tree = result.tree()
        data = StructuredData(tree.children()[0])
        names = list(el for el in data._imported_attrs if el not in ("count",))
        ListData = namedtuple("ListData", names)

        vals = []
        for i in range(data.count):
            row = {}
            for name in names:
                try:
                    row[name] = getattr(data, name)[i]
                except (TypeError, IndexError):
                    row[name] = getattr(data, name)
            vals.append(ListData(**row))
        return vals

    def list_signals(self, **kwargs):
        """
        List available signals.

        See Client.list for arguments.
        :return: A list of namedtuples returned signals
        """
        return self.list(ListType.SIGNALS, **kwargs)

    def __getattr__(self, item):
        if item in self._registered_subclients:
            return getattr(self._registered_subclients[item], item)

        raise AttributeError("%s method not found in registered subclients" % item)

    @classmethod
    def get_property(cls, prop):
        return cpyuda.get_property(prop)

    @classmethod
    def set_property(cls, prop, value):
        cpyuda.set_property(prop, value)
