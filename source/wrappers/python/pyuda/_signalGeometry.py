from __future__ import (division, unicode_literals, print_function, absolute_import)

from ._data import Data

import inspect

from future import standard_library
standard_library.install_aliases()


class SignalGeometryData(Data):
    """
    Class to import signal geometry data
    """

    def __init__(self, sigData, signal, all=False):
        """
        Initialisation
        :param sigData:
        :param kwargs:
        :return:
        """
        self._sdata = sigData
        self._signal = signal
        self.data = None
        self._keep_all = all

        self._import_data()


    def _import_data(self):
        """

        :return:
        """
        signal_type = self._sdata["data"].signal_type

        # If it is an element, keep only the relevant one.
        self.data = self._sdata

        # Get rid of extra data groups
        if signal_type == "group":
            self.data["data"].delete_level("data")
            self.data["data"].delete_attr("signal_type")
            self.data.delete_level("data")
        else:
            self.data.delete_level("data")
            self.data.delete_attr("signal_type")

        if not self._keep_all and hasattr(self.data, "signal_alias_available"):
            self.data.display(depth=2)
            available_signals = self.data.signal_alias_available
            if signal_type == "group":
                self._loop_to_remove_signals(available_signals)
            else:
                if self.data.signal_alias not in self.data.signal_alias_available:
                    self.data = None

    def get_all_geom_names(self, data=None):
        if self.data is None:
            return []

        if data is None:
            data = self.data

        all_names = []
        comp_names = []

        attributes = self._get_all_attr(data, exclude=('children', 'name'))

        for attr in attributes:
            if attr[0] == "comp_names":
                names = [a.replace(" ", "") for a in attr[1].split(',')]
                comp_names.extend(names)

        if len(comp_names) > 0:
            all_names.extend(comp_names)
        else:
            for child in data.children:
                all_names.extend(self.get_all_geom_names(data=child))

        return all_names

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

    def add_geom_data(self, geom_data, lead_group):
        """

        :param geom_data:
        :return:
        """
        if lead_group[0] != "/":
            lead_group = "".join(["/", lead_group])

        self._loop_to_add_geom_data(geom_data, lead_group)

    def _loop_to_add_geom_data(self, geom_data, lead_group, this_data=None):
        if self.data is None:
            return

        if geom_data is None:
            return

        if this_data is None:
            this_data = self.data

        if hasattr(this_data, "comp_names"):
            geom_to_add = this_data.comp_names.split(',')
            geom_signals_to_add = []

            for geom_name in geom_to_add:
                geom_name = geom_name.replace(" ", "")
                if geom_name.find(lead_group) != 0:
                    continue

                if geom_data.data.signal_type == "group":
                    geom_access = geom_name.replace(lead_group, "")
                    geom_name = geom_access[geom_access.rfind('/')+1:]
                else:
                    geom_access = ""
                    geom_name = geom_name[geom_name.rfind('/')+1:]

                geom_array = geom_data.data[geom_access]
                all_names = [child.name_ for child in geom_array.children]

                if geom_name in all_names:
                    ind_match = all_names.index(geom_name)
                    geom_signals_to_add.append(geom_array.children[ind_match])

            if len(geom_signals_to_add) > 0:
                this_data.add_attr("geomsignals", geom_signals_to_add)
        else:
            for child in this_data.children:
                self._loop_to_add_geom_data(geom_data, lead_group, this_data=child)

    def _loop_to_remove_signals(self, available_signals, this_data=None):
        """
        Recursively loop over tree, looking for the level where the signal data is.
        Check if the signal is in the list of available signals.
        Delete those signals that aren't available
        :param available_signals: List of available signal names
        :param this_data: The current SignalGeometry
        :return:
        """

        if self.data is None:
            return

        if this_data is None:
            this_data = self.data

        signal_level = False

        for child in this_data.children:
            if hasattr(child, "dimensions"):
                signal_level = True

        if signal_level:
            ind_keep = []

            for index, child in enumerate(this_data.children):
                if not hasattr(child["data"], "signal_alias"):
                    continue

                if child["data"].signal_alias in available_signals:
                    ind_keep.append(index)

            if len(ind_keep) > 0:
                this_data.retain_children(ind_keep)
        else:
            for child in this_data.children:
                self._loop_to_remove_signals(available_signals, this_data=child)

    def plot(self):
        raise NotImplementedError("plot function not implemented for SignalGeometryData objects")

    def widget(self):
        raise NotImplementedError("widget function not implemented for SignalGeometryData objects")