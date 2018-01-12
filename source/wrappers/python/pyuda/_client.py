import logging

from six import add_metaclass
from . import c_uda
from ._signal import Signal
from ._string import String
from ._structured import StructuredData
from ._structuredWritable import StructuredWritable
from ._geometryFiles import GeometryFiles
from ._geometry import GeometryData
from ._signalGeometry import SignalGeometryData


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


@add_metaclass(ClientMeta)
class Client(object):
    """
    A class representing the IDAM client.

    This is a pythonic wrapper around the low level c_uda.Client class which contains the wrapped C++ calls to
    IDAM.
    """

    def __init__(self, debug_level=logging.ERROR):
        self._cclient = c_uda.Client()

        logging.basicConfig(level = debug_level)

        self.logger = logging.getLogger(__name__)
        
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

        if result.isTree():
            return StructuredData(result.tree())
        elif result.type() == 'string':
            return String(result)
        return Signal(result)

    def _get_signal_filenames(self, geom_signals, shot, **kwargs):
        """
        Given a geometry signal or group, query IDAM to retrieve the filenames
        of the netcdf files that contain the corresponding data signal mapping info.
        Also returns the geom signal - data signal mapping.
        :param geom_signals: Either a geometry signal or group to retrieve signal mapping info.
        :param shot: Shot number
        :param kwargs: Optional arguments: version_signal can be specified to retrieve a specific version of the file
        :return:
        """
        signal_call = "GEOM::getSignalFilename(geomsignal='{}'".format(geom_signals)
        if "version_signal" in kwargs.keys():
            signal_call = signal_call + ", version={}".format(kwargs["version_signal"])
        signal_call += ')'

        try:
            self.logger.info("Call to get signal filenames is {} for shot {}".format(signal_call, shot))
            signal_filename_data = StructuredData(self._cclient.get(str(signal_call), shot).tree())

            filenames = signal_filename_data["data"].filenames
            geom_alias = signal_filename_data["data"].geom_alias
            signal_alias = signal_filename_data["data"].signal_alias
            var_name = signal_filename_data["data"].var_name

            return filenames, geom_alias, signal_alias, var_name

        except c_uda.UDAException:
            return None, None, None, None

    def _find_matching_group(self, signal_aliases):
        """
        Helper function. Take a list of signals, and find the highest level group common to all of them.
        Assumes groups are separated by "/".
        :param signal_aliases: List of signals
        :return:
        """
        if len(signal_aliases) == 1:
            return signal_aliases[0]

        to_compare = signal_aliases[0].split("/")
        the_rest = []

        for signals in signal_aliases[1:]:
            the_rest.extend(signals.split("/"))

        match = ""

        for comp in to_compare:
            if comp == "":
                continue

            if the_rest.count(comp) == to_compare.count(comp)*(len(signal_aliases)-1):
                match = match + "/" + comp
            else:
                break

        return match[1:]

    def signal_geometry(self, signal, source, keep_all=False, **kwargs):
        """
        Retrieve data signal - geometry signal mapping and info for a particular data signal or group.
        First, retrieves signal - geometry signal mapping, then retrieves geometry signal info and combines.
        :param signal: Data signal or group.
        :param source: Shot to retrieve info for
        :param kwargs: Optional arguments.
        :return:
        """
        self.logger.info("Retrieving geometry info associated with signals")

        # Retrieve data signal - geometry signal mapping
        sig_call = "GEOM::getSignalFile(signal='{0}'".format(signal.lower())
        source_call = source

        if "version_signal" in kwargs.keys():
            sig_call += sig_call+"version={}".format(kwargs["version_signal"])
        if keep_all:
            sig_call = "".join([sig_call, ", keep_all=True"])
        sig_call += ")"

        try:
            self.logger.info("Call is {}".format(sig_call))
            sig_struct = StructuredWritable(self._cclient.get(str(sig_call), str(source_call)).tree())
        except c_uda.UDAException:
            self.logger.error("Could not retrieve signal geometry data for signal {} and source {}".format(signal,
                                                                                                           source))
            return

        sig_struct = SignalGeometryData(sig_struct, signal, all=keep_all)

        # Extract names of the geometry signals associated with the given data signal or group
        geom_names = sig_struct.get_all_geom_names()

        if len(geom_names) == 0:
            return sig_struct

        # Find lowest matching group
        matching_group = self._find_matching_group(geom_names)

        # Retrieve geometry data
        geom_data = self.geometry(matching_group, source, **kwargs)

        # Add geometry data to signal data
        sig_struct.add_geom_data(geom_data, matching_group)

        return sig_struct

    def geometry(self, signal, source, toroidal_angle=None, no_cal=False, **kwargs):
        """
        Retrieve geometry data for a given geometry signal or group.
        If a calibration file is available, this will be retrieved & the geometry data will be calibrated.
        If requested (add_signals=True) then the mapping to the data signals will also be retrieved, and that
         info inserted into the structure in the appropriate places
        :param signal: Geometry signal or group
        :param source: Shot to retrieve info for
        :param toroidal_angle: Toroidal angle at which to retrieve info.
                               (If geometry component has no toroidal dependence this is not required.)
        :param no_cal: Don't apply geometry calibration if set to True
        :param kwargs: Optional arguments.
        :return:
        """

        # Get rid of trailing slash...
        if signal[-1] == '/':
            signal = signal[:-1]

        # Retrieve signal names and modules to be used for
        # manipulating the data
        filenames_call = "GEOM::getConfigFilenames(signal={})".format(signal.lower())

        self.logger.debug("Call to retrieve filenames is {}".format(filenames_call))

        multiple_names = self.get(filenames_call, source)

        signal_map = GeometryFiles()
        signal_groups = multiple_names["data"].geomgroups
        signal_groups = list(set(signal_groups))

        manip = signal_map.get_signals(signal_groups)

        self.logger.debug("Signal groups: {}".format(signal_groups))

        if len(signal_groups) > 1:
            signal_names = signal_groups
        else:
            signal_names = [signal.lower()]

        if signal_names is None:
            return

        results = []
        signal_filenames = []
        geom_aliases = []
        signal_aliases = []
        signal_var_names = []

        source_call = source
        geom_call = "GEOM::get(signal={0}, {1}, {2}"

        if not isinstance(source, (int)) and source != "":
            geom_call = geom_call+(", file={0}".format(source_call))

        if toroidal_angle is not None:
            geom_call = geom_call+(", tor_angle={0})".format(toroidal_angle))
        else:
            geom_call = geom_call+")"

        version_config = ""
        version_cal = ""
        if "version_config" in kwargs.keys():
            version_config = "version={0}".format(kwargs["version_config"])
        if "version_cal" in kwargs.keys():
            version_cal = "version_cal={0}, {1}".format(kwargs["version_cal"], version_config)

        # Loop files to be read in
        for signal_name in signal_names:
            if signal_name[-1] == '/':
                signal_name = signal_name[:-1]

            self.logger.debug("Retrieve geom data for {}".format(signal_name))

            # Get configuration data
            config_extra = "Config=1"
            config_call = geom_call.format(signal_name, config_extra, version_config)

            self.logger.info("Call is {0}\n".format(config_call))
            try:
                config_struct = StructuredWritable((self._cclient.get(str(config_call), str(source_call))).tree())
            except c_uda.UDAException:
                self.logger.error("ERROR: Could not retrieve geometry data for signal {0} and source {1}".format(signal,
                                                                                                         source))
                return

            # Get calibration data unless asked not to calibrate
            if not no_cal:
                cal_extra = "cal=1"
                cal_call = geom_call.format(signal_name, cal_extra, version_cal)
                self.logger.debug("Call is {0}\n".format(cal_call))
                try:
                    cal_struct = StructuredWritable((self._cclient.get(str(cal_call), str(source_call))).tree())
                    self.logger.debug("Calibration data was found")
                except c_uda.UDAException:
                    cal_struct = None
                    self.logger.debug("No calibration data was found")
            else:
                cal_struct = None

            results.append([config_struct, cal_struct])

            # Get filenames for data signal info if asked for
            if "add_signals" in kwargs.keys():
                s_filenames, g_aliases, s_aliases, s_var_names = self._get_signal_filenames(signal_name,
                                                                                str(source), **kwargs)
                if s_filenames is not None:
                    signal_filenames.extend(s_filenames)
                    geom_aliases.extend(g_aliases)
                    signal_aliases.extend(s_aliases)
                    signal_var_names.extend(s_var_names)

        # Calibrate geometry data
        geom_data = GeometryData(results, signal_names, manip, **kwargs)

        # If asked for, use signal filenames retrieved previously to retrieve signals and add to the structure
        if "add_signals" not in kwargs.keys():
            return geom_data
        elif kwargs["add_signals"] and len(signal_filenames) > 0:
            self.logger.info("Adding signals to geometry information")

            # Find unique filenames and lowest matching group
            signal_filenames = list(set(signal_filenames))
            global_signal_group = self._find_matching_group(signal_var_names)
            if global_signal_group[-1] == "/":
                global_signal_group = global_signal_group[0:-1]

            signal_data = None
            for signal_file in signal_filenames:
                try:
                    self.logger.info("Retrieving signal data, signal is {} file is {} group {}".format(global_signal_group,
                                                                                              signal_file, global_signal_group))
                    if signal_data is None:
                        signal_data = StructuredWritable((self._cclient.get(global_signal_group, signal_file)).tree())
                    else:
                        signal_data._add_struct(StructuredWritable((self._cclient.get(global_signal_group,
                                                                                      signal_file)).tree()))
                except c_uda.UDAException:
                    self.logger.warning("Something went wrong retrieving signal data")
                    continue

            # Add signals to geometry data
            geom_data.add_signal_data(signal_data, geom_aliases, signal_aliases, signal_var_names,
                                      signal, global_signal_group)

            return geom_data
        else:
            return geom_data

    @classmethod
    def get_property(cls, prop):
        return cls.C_Client.property(prop)

    @classmethod
    def set_property(cls, prop, value):
        cls.C_Client.setProperty(prop, value)        
