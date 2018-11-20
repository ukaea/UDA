from __future__ import (division, print_function, absolute_import)

from . import c_uda
from ._signal import Signal
from ._string import String
from ._structured import StructuredData

import logging
import itertools
from collections import namedtuple
try:
    from enum import Enum
except ImportError:
    Enum = object

from builtins import (range, int, bytes)
from future import standard_library
from future.utils import with_metaclass
standard_library.install_aliases()


class ClientMeta(type):
    """
    Metaclass used to add class level properties
    """
    def __init__(cls, what, bases=None, dict=None):
        type.__init__(cls, what, bases, dict)
        cls.C_Client = c_uda.Client

    @property
    def port(cls):
        return cls.C_Client.serverPort()

    @port.setter
    def port(cls, value):
        cls.C_Client.setServerPort(int(value))

    @property
    def server(cls):
        return cls.C_Client.serverHostName()

    @server.setter
    def server(cls, value):
        cls.C_Client.setServerHostName(value)


class ListType(Enum):
    SIGNALS = 1
    SOURCES = 2
    SHOTS = 3


class Client(with_metaclass(ClientMeta, object)):
    """
    A class representing the IDAM client.

    This is a pythonic wrapper around the low level c_uda.Client class which contains the wrapped C++ calls to
    IDAM.
    """

    def __init__(self, debug_level=logging.ERROR):
        self._cclient = c_uda.Client()
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
        
    def get(self, signal, source, **kwargs):
        """
        IDAM get data method.

        :param signal: the name of the signal to get
        :param source: the source of the signal
        :param kwargs: additional optional keywords for geometry data
        :return: a subclass of pyuda.Data
        """
        # Standard signal
        result = self._cclient.get(str(signal), str(source))

        if 'raw' in kwargs and kwargs['raw']:
            data = result.data()
            byte_array = c_uda.ByteArray.frompointer(data.byte_data())
            return bytes(itertools.islice(byte_array, data.byte_length()))

        if result.isTree():
            return StructuredData(result.tree())
        elif result.type() == 'string':
            return String(result)
        return Signal(result)

    def list(self, list_type, shot=None, alias=None, signal_type=None):
        """
        Query the server for available data.

        :param list_type: the type of data to list, must be one of pyuda.ListType
        :param shot: the shot number, or None to return for all shots
        :param alias: the device alias, or None to return for all devices
        :param signal_type: the signal types {A|R\M|I}, or None to return for all types
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

        args += list_arg

        result = self._cclient.get("meta::list(context=data, cast=column, %s)" % args, "")
        if not result.isTree():
            raise RuntimeError("UDA list data failed")

        data = StructuredData(result.tree())
        names = list(el for el in data["data"]._imported_attrs if el not in ("count",))
        ListData = namedtuple("ListData", names)

        vals = []
        for i in range(data["data"].count):
            row = {}
            for name in names:
                try:
                    row[name] = getattr(data["data"], name)[i]
                except TypeError:
                    row[name] = getattr(data["data"], name)
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
        return cls.C_Client.property(prop)

    @classmethod
    def set_property(cls, prop, value):
        cls.C_Client.setProperty(prop, value)        
