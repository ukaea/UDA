"""
Class to import geometry data.
1. Combines configuration and calibration data
2. Applies any manipulations required for that file.
   (a class must be passed in with methods do_manip to
    do the manipulation and plot to plot the components)

PROBLEMS:
 1. Enums come back as longs... Not very useful for telling people
    stuff. Also, bad since then they are treated as numbers and code
    will try to add them when calibrating... Perhaps should ditch the
    enums and use strings instead?
"""

import inspect
import logging

import numpy as np

from ._data import Data

class GeometryData(Data):
    def __init__(self, structData, signal_config, manip, **kwargs):
        """
        Initialisation
        :param structData: A list of pairs of calibration and configuration StructuredData objects
        :param signal_config: Signal name requested
        :param manip: A list of manipulator classes for each set of files.
                      If an element is None then no manipulation is done.
        :param kwargs: Keyword arguments to be passed to manipulators
        :return:
        """
        self._sdata = structData
        self._signal_config = signal_config
        self._manip = manip
        self._manip_kw = kwargs
        self.data = None
        self._logger = logging.getLogger(__name__)
        self._import_data()

    def _import_data(self):
        """
        Take the pairs of StructuredData that
        were passed to the class, calibrate the
        data, perform any manipulations required
        and combine into one structure.
        :return:
        """
        for main_node, signal_name, manipulator in zip(self._sdata, self._signal_config, self._manip):
            if signal_name[-1] == '/':
                signal_name = signal_name[:-1]

            self._logger.debug("Applying geometry calibration for signal_name {0}".format(signal_name))
            config_data = main_node[0]
            signal_type = config_data["data"].signal_type

            # Calibration file was available
            # make combinations where appropriate
            if main_node[1] is not None:
                self._logger.debug("There is calibration data.")
                cal_data = main_node[1]
                self._child_loop(cal_data, config_data, signal_name, signal_type)

            self._logger.debug("Applying manipulations")

            # Manipulations to return the correct aspect of the data.
            if manipulator is not None:
                self._logger.debug("There are manipulations to be done for this data")
                manipulator.do_manip(config_data, **self._manip_kw)

            self._logger.debug("Deleting levels")

            # Get rid of some unnecessary levels in the tree
            group_name = signal_name[signal_name.rfind('/')+1:]
            config_data.change_child_name("data", group_name)

            if signal_type == "group" or signal_type == "array":
                config_data[group_name].delete_level("data")

            if len(self._sdata) == 1:
                config_data.delete_level(group_name)
            else:
                config_data[group_name].delete_attr("signal_type")

            self._logger.debug("Storing data")

            # Store the data
            if self.data is None:
                self.data = config_data
            else:
                self._add_struct(config_data)

    def _get_all_attr(self, data, exclude=()):
        """
        Get all attributes except those in exclude
        :param data: class
        :param exclude: attributes to be excluded from list
        :return: list of attributes of the class
        """
        attr_data = inspect.getmembers(data, lambda a: not (inspect.isroutine(a)))
        attr_data = [a for a in attr_data
                    if not (a[0].startswith('_') or a[0] in exclude)]

        return attr_data

    def _child_loop(self, cal_data, config_data, signal_name, signal_type):
        """
        Recursively loop over children in the cal tree.
        If an attribute doesn't exist in the Config tree
        it is added.
        If the user has asked for a variable, or an element
        of a variable then this is calibrated and returned,
        if a calibration exists.
        If the user has asked for a group of variables, then
        the children are looped over, looking for variables
        that need to be calibrated.
        The assumption is that a variable containing calibration
        data ends with "_cal".
        :param cal_data: calibration data (instance of StructuredWritable)
        :param config_data: configuration data (instance of StructuredWritable)
        :param signal_name: signal name that was asked for.
        :param signal_type: 'group', 'array' or 'element'.
        :return:
        """

        # Attributes
        attr_cal = self._get_all_attr(cal_data, exclude=('children', 'name'))
        attr_config = self._get_all_attr(config_data, exclude=('children', 'name'))
        attr_names_config = [a[0] for a in attr_config]

        for attr in attr_cal:
            # If there is an attribute in the calibration,
            # but not in the configuration, add it to the configuration.
            if attr[0] not in attr_names_config:
                # Add it in
                config_data.add_attr(attr[0], attr[1])

        if signal_type == "element":
            # We've already retrieved the correct data -> calibrate!
            # NB: Strings coming back wrong here, otherwise we could double-check
            self._calibrate_data(cal_data["data/data"], config_data["data/data"])
        else:
            # They've asked for a group of variables:
            # Loop over children and check for data that needs calibrating.
            children_names_config = [child.name for child in config_data.children]

            for child in cal_data.children:
                if (hasattr(child, 'calibration')):
                    if child.calibration == "True" and child.name in children_names_config:
                        # Found data to be calibrated: find matching Config data & calibrate
                        child_ind = children_names_config.index(child.name)
                        self._calibrate_data(child["data"], (config_data.children[child_ind])["data"])
                    else:
                        # This is a group that is not in the Config file and/or is not calibration data: add it.
                        config_data.add_child(child)
                elif child.name in children_names_config:
                    # Child is in calibration and Config data, continue looping to look
                    # for variable that needs calibrating
                    child_ind = children_names_config.index(child.name)
                    self._child_loop(child, config_data.children[child_ind], signal_name, signal_type)

    def _calibrate_data(self, cal_data, config_data):
        """
        Calibrate configuration data using cal data.
        :param cal_data: calibration data
        :param config_data: configuration data
        :return:
        """
        cal_type = cal_data.type
        replace_values = (cal_type == "ABSOLUTE")
        self._correct_loop(cal_data, config_data, replace=replace_values)

    def _correct_loop(self, cal_data, config_data, replace=False):
        """
        Recursively loop over all children and attributes.
        If an attribute exists in calibration and configuration
        data then the configuration data is modified using the
        calibration data.
        If an attribute or child doesn't exist in the configuration
        data it is added to the structure.
        :param cal_data: calibration data (instance of StructuredWritable)
        :param config_data: configuration data (instance of StructuredWritable)
        :param replace: True, calibration data replaces configuration data
                        False, calibration data is summed with configuration data.
                        (ie. relative or absolulte calibration)
        :return:
        """
        # Attributes
        attr_cal = self._get_all_attr(cal_data, exclude=('children', 'name', 'type', 'status'))
        attr_config = self._get_all_attr(config_data, exclude=('children', 'name', 'type', 'status'))
        attr_names_config = [a[0] for a in attr_config]

        for attr in attr_cal:

            # If there is an attribute in the calibration,
            # but not in the configuration, add it to the configuration.
            is_numeric = isinstance(attr[1], (int, float, np.ndarray))
            if isinstance(attr[1], (np.ndarray)):
                is_numeric = np.issubdtype(attr[1].dtype, np.number)

            if attr[0] in attr_names_config and is_numeric:
                # Get Config data
                if replace:
                    setattr(config_data, attr[0], attr[1])
                else:
                    config = getattr(config_data, attr[0])
                    setattr(config_data, attr[0], config+attr[1])

            elif attr[0] not in attr_names_config:
                config_data.add_attr(attr[0], attr[1])

        # Loop over children and check for matching data in Config.
        children_names_config = [child.name for child in config_data.children]

        for child in cal_data.children:

            if child.name in children_names_config:
                child_ind = children_names_config.index(child.name)
                self._correct_loop(child, config_data.children[child_ind])
            else:
                config_data.add_child(child)

    def _add_struct(self, data):
        """
        Add data struct to self.data
        at the level below ROOT level.
        :param data: StructuredData object to add
        :return:
        """
        for child in data.children:
            self.data.add_child(child)

        self.data.count += len(data.children)
        self.data.shape += len(data.children)

    def get_all_geom_names(self, data=None, current_level=""):
        """
        Recursively extract the full paths of all geometry signals
        :param data: StructuredWritable class instance, at the current level of the tree.
                     If set to None, start at the top of the tree.
        :param current_level: Add current_level to the start of each path.
        :return:
        """
        all_names = []

        if data is None:
            data = self.data

        attributes = self._get_all_attr(data, exclude=('children', 'name'))
        geom_names = [current_level for a in attributes if a[0] == "name_"]

        if len(geom_names) > 0:
            all_names.extend(geom_names)
        else:
            for child in data.children:
                level_here = current_level
                if child.name != "data":
                    level_here = current_level+"/"+child.name
                all_names.extend(self.get_all_geom_names(data=child, current_level=level_here))

        return all_names

    def add_signal_data(self, signal_data, geom_aliases, signal_aliases, signal_var_names, #signal_types,
                        geom_signal_call, global_signal_group):
        """
        Insert signal data into appropriate locations for geom_aliases.
        :param signal_data: Signal data class, containing the signals to be added
        :param geom_aliases: The full paths of the geometry signals with associated signals.
        :param signal_aliases: The signal names of the signals associated with the geometry signals.
        :param signal_types: The paths of the signals associated with the geometry signals.
        :param geom_signal_call: geometry signal that was requested.
        :param global_signal_group: The group which was requested when retrieving the signal_data.
        :return:
        """

        # Loop over geometry aliases which have signals in signal data
        geom_alias_unique = set(geom_aliases)
        for geom_alias in geom_alias_unique:
            ind_alias = [index for index, g_alias in enumerate(geom_aliases) if g_alias == geom_alias]
            sig_var_names = [signal_var_names[index] for index in ind_alias]
            sig_alias = [signal_aliases[index] for index in ind_alias]

            # String to access correct node for this geom signal
            geom_access = geom_alias[len(geom_signal_call):]+"/data"

            signals_to_add = []
            # Retrieve the appropriate signals from the signal data
            for s_alias, s_var_name in zip(sig_alias, sig_var_names):
                signal_access = "".join(["/data"+s_var_name[len(global_signal_group)+1:]])

                if signal_access == "/data" or signal_access == "/data/":
                    signal_access = ""

                signal_array = signal_data[signal_access]

                sig_child_names = [child.signal_alias if child.signal_alias[-1] != '/' else child.signal_alias[0:-1]
                                       for child in signal_array.children]

                if s_alias[-1] == '/':
                    s_alias = s_alias[0:-1]

                if s_alias.lower() in sig_child_names:
                    signals_to_add.append(signal_array.children[sig_child_names.index(s_alias.lower())])

            # Add the signal data to the appropriate geometry element
            if len(signals_to_add) > 0:
                self.data[geom_access].add_attr("signals", signals_to_add)

    def plot(self, ax_2d=None, ax_3d=None, show=True, color=None):
        """
        Plot components in 2D and 3D.
        :param ax_2d: Axis on which to plot location of components in R-Z (2D) plane.
                      If None, then an axis will be created.
        :param ax_3d: Axis on which to plot location of components in x-y-z (3D) plane.
                      If None, then an axis will be created.
        :return:
        """
        import matplotlib.pyplot as plt
        from mpl_toolkits.mplot3d import Axes3D
    
        self._manip
        if len(self._manip) > 1:
            fig = plt.figure()
            ax_2d = fig.add_subplot(121)
            ax_3d = fig.add_subplot(122, projection='3d')

            for index, manipulator in enumerate(self._manip):
                if manipulator is None:
                    continue

                manipulator.plot(self.data.children[index], ax_2d=ax_2d, ax_3d=ax_3d, show=False, color=color)

            if show:
                plt.show()
        else:
            if ax_2d is None and ax_3d is None:
                plt.close()
                fig = plt.figure()
                ax_2d = fig.add_subplot(121)
                ax_3d = fig.add_subplot(122, projection='3d')

            self._manip[0].plot(self.data, ax_2d=ax_2d, ax_3d=ax_3d, show=show, color=color)


    def widget(self):
        raise NotImplementedError("widget function not implemented for GeometryData objects")