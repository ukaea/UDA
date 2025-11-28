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
import math
from collections import defaultdict
from progress.bar import Bar
from collections.abc import Iterable
import sys
import importlib
import warnings
try:
    from enum import Enum
except ImportError:
    Enum = object


class UdaSubclientDeprecationWarning(UserWarning):
    def __init__(self, message):
        super().__init__(UdaSubclientDeprecationWarning, message)


class UdaSubclientInterfaceError(cpyuda.UDAException):
    pass


class UdaSubclientsStringError(cpyuda.UDAException):
    pass


def _parse_subclient_register_from_env():
    """
     Parse a list of uda subclient strings from an environment variable
     into a map of {module_path: [subclient_classes,]}

     Input string should be formatted as a colon-delimited list,
     with each entry containing the module path to the subclient class,
     as it would be written to import in python
     e.g. UDA_SUBCLIENTS=mast.MastClient:mast.geom.GeometryClient:another_module.AnotherSubClient

     Throws: UdaSubclientsStringError if the UDA_SUBCLIENTS string is misformed (e.g. missing required . char)
     Throws: KeyError if the environment variable does not exist

    """
    import re
    import os
    subclients_string = os.environ["UDA_SUBCLIENTS"]
    if subclients_string == "":
        return {}

    string_validator = re.fullmatch(r'(([a-zA-z]\w*)(\.[a-zA-z]\w*)+:?)+', subclients_string)
    if string_validator is None:
        raise UdaSubclientsStringError("UDA_SUBCLIENTS string is incorrectly formatted")

    entries = [i for i in subclients_string.split(':') if i != '']
    subclient_register = defaultdict(list)
    for entry in entries:
        module_path, subclient_class = entry.rsplit('.', maxsplit=1)
        subclient_register[module_path].append(subclient_class)
    return dict(subclient_register)


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


def _target(server, port, queue):
    Client.port = port
    Client.server = server
    try:
        result = cpyuda.get_data("help::help()", "")
        queue.put(result.is_string())
    except cpyuda.ServerException as ex:
        print("cpyuda.ServerException: %s" % ex)


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
        self.register_all_subclients()

    def _parse_subclient_register_from_yaml(self):
        import yaml
        subclient_register_path = os.environ["UDA_SUBCLIENT_REGISTER"]
        with open(subclient_register_path, 'r') as file:
            return yaml.safe_load(file)

    def register_all_subclients(self):
        """
        Method attempts to obtain a list of uda subclients from an environment variable
        and register the exported methods for use in a standard pyuda client. 

        This currently falls back to the (mast-specific) legacy registration routine to 
        maintain previous behaviour where still required.

        behaviour:
            - no environment variable set: fallback to legacy registration routine
            - list provided is incorrectly formatted: fallback to legacy registration routine
            - list provided is empty: no subclients registered
            - subclient does not provide a register method: fallback to legacy routine
        """
        try:
            subclient_register = _parse_subclient_register_from_env()
        except UdaSubclientsStringError:
            warnings.warn("WARNING: cannot parse UDA_SUBCLIENTS string as it is incorrectly formatted. "
                          "Falling back to legacy subclient registration method. This behaviour will be "
                          "deprecated along with the fallback method in a later release",
                          UdaSubclientDeprecationWarning)
            self.register_legacy_subclients()
            return
        except KeyError:
            warnings.warn("WARNING: UDA_SUBCLIENTS environment variable not set. Falling back to "
                          " legacy subclient registration method. This behaviour will be deprecated "
                          "along with with the fallback method in a later release",
                          UdaSubclientDeprecationWarning)
            self.register_legacy_subclients()
            return

        try:
            for module_name in subclient_register:
                module = importlib.import_module(module_name)
                subclient_names = subclient_register[module_name]

                # assume either one or multiple subclients may be specified in each subclient module
                if type(subclient_names) is not list:
                    subclient_names = [subclient_names]

                for subclient_name in subclient_names:
                    subclient = getattr(module, subclient_name)
                    self.register_subclient(subclient)
        except UdaSubclientInterfaceError:
            warnings.warn("WARNING: one of the subclient classes specified did not provide "
                          "a register method. This may be caused by an old version which does "
                          "not conform to the new interface. Falling back to legacy subclient "
                          "registration method. This behaviour will be deprecated along with the "
                          "fallback method in a later release", UdaSubclientDeprecationWarning)
            self.register_legacy_subclients

    def register_legacy_subclients(self):
        # this warning will annoy all non-mast users until we deprecate
        warnings.warn("WARNING: The pyuda client has fallen back to using the legacy "
                      "subclient registration routine, possibly "
                      " because the UDA_SUBCLIENTS "
                      "environment variable has not been set, "
                      "or was incorrectly formatted. \n\n"
                      "Note that any errors encountered importing the mast module "
                      "used in this legacy routine will not be reported, which "
                      "will frustrate debugging if you require this functionality\n\n"
                      "This legacy (mast-specific) routine will be deprecated "
                      "in a future release. \n\n"
                      "Consider using the UDA_SUBCLIENTS "
                      "environment variable if your code relies on subclient "
                      "features such as list_signals.\n\n"
                      "You can disable this warning using "
                      "warnings.simplefilter('ignore', pyuda.UdaSubclientDeprecationWarning) "
                      "or by setting the UDA_SUBCLIENTS variable with an empty string",
                      UdaSubclientDeprecationWarning)
        try:
            from mast.geom import GeomClient
            from mast import MastClient
            mast_client = MastClient(self)
            geom_client = GeomClient(self)
            self._registered_subclients['geometry'] = geom_client
            self._registered_subclients['geometry_signal_mapping'] = geom_client
            self._registered_subclients['get_images'] = mast_client
            self._registered_subclients['get_shot_date_time'] = mast_client
            self._registered_subclients['latest_shot'] = mast_client
            self._registered_subclients['latest_source_pass'] = mast_client
            self._registered_subclients['latest_source_pass_in_range'] = mast_client            
            self._registered_subclients['listGeomSignals'] = geom_client
            self._registered_subclients['listGeomGroups'] = geom_client
            self._registered_subclients['list'] = mast_client
            self._registered_subclients['list_archive_files'] = mast_client
            self._registered_subclients['list_archive_directories'] = mast_client
            self._registered_subclients['list_file_signals'] = mast_client            
            self._registered_subclients['list_signals'] = mast_client
            self._registered_subclients['put'] = mast_client
            self._registered_subclients['list_shots'] = mast_client
            self._registered_subclients['list_sources'] = mast_client
        except ImportError:
            pass

    def register_subclient(self, subclient_class):
        if not hasattr(subclient_class, "register"):
            raise UdaSubclientInterfaceError("The subclient class specified does not provide a \"register\" method")
        subclient_class.register(self)

    def register_method(self, method, subclient_instance):
        if method in self._registered_subclients:
            previous = self._registered_subclients[method].__name__
            warnings.warn(f"The subclient method \"{method.__name__}\" previously registered to {previous} "
                          f"has been overwritten by {subclient_instance.__name__}")
        self._registered_subclients[method] = subclient_instance

    def get_file(self, source_file, output_file=None, chunk_size=1):
        """
        Retrieve file using bytes plugin and write to file
        :param str      source_file: the full path to the file
        :param str|None output_file: the name of the output file
        :param int      chunk_size: download chunk size in MB, set to 0 to download the file in one chunk
        :return:
        """
        if chunk_size < 0:
            raise ValueError("chunk_size must not be negative")

        # bytes::size() function won't exist in some old servers,
        # check for compatible plugin version.
        # automatic versioning was introduced for bytes plugin
        # in release 2.8.1, this changed the return type from int to str
        result = cpyuda.get_data("bytes::version()", "")
        if not result.is_string():
            chunk_size = 0

        if chunk_size:
            result = cpyuda.get_data("bytes::size(path={path})".format(path=source_file), "")
            size = result.data()
            chunk_size = int(chunk_size * 1024 * 1024)
            count = 0
            steps = math.ceil(size / chunk_size)
            bar = Bar('Downloading', max=steps, suffix='%(percent)d%%')
            with open(output_file, 'wb') as f_out:
                while count < size:
                    result = cpyuda.get_data("bytes::read(path={path}, max_bytes={max_bytes}, offset={offset}, /opaque)".format(path=source_file, max_bytes=chunk_size, offset=count), "")
                    data = result.data()
                    count += data.size
                    data.tofile(f_out)
                    bar.next()
            print(flush=True)
        else:
            result = cpyuda.get_data("bytes::read(path={path}, /opaque)".format(path=source_file), "")

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

    @classmethod
    def close_connection(cls):
        cpyuda.close_connection()

    @classmethod
    def reset_connection(cls):
        cpyuda.close_connection()

    @classmethod
    def test_connection(cls, timeout=1):
        import multiprocessing
        queue = multiprocessing.Queue()
        p = multiprocessing.Process(target=_target, args=(cls.server, cls.port, queue))
        p.start()
        p.join(timeout)
        if p.is_alive():
            p.terminate()
            p.join()
            raise TimeoutError("Connection test timed out after %1.2f seconds"
                               % timeout)
        if queue.empty():
            return False
        return queue.get()

    @classmethod
    def query_server_version(cls):
        result = cpyuda.get_data("help::version()", "")
        if not result.is_string():
            warnings.warn("Server versions before 2.8.1 do not report their software version through this interface")
            return None
        return result.data()

    @classmethod
    def query_server_info(cls):
        software_version = cls.query_server_version()
        protocol_version = cpyuda.get_server_protocol_version()
        uuid = cpyuda.get_server_uuid()
        return {"software_version": software_version, "protocol_version": protocol_version,
                "uuid": uuid, "server": cls.server, "port": cls.port}
