from __future__ import (division, unicode_literals, print_function, absolute_import)

from ._utils import cdata_to_numpy_array

from future import standard_library
standard_library.install_aliases()


class Dim(object):

    def __init__(self, cdim):
        self._cdim = cdim
        self._data = None
        self._label = None
        self._units = None

    @property
    def data(self):
        if self._data is None and self._cdim is not None:
            self._import_data()
        return self._data

    def _import_data(self):
        if not self._cdim.isNull():
            self._data = cdata_to_numpy_array(self._cdim)

    @property
    def label(self):
        if self._label is None and self._cdim is not None:
            self._label = self._cdim.label()
        return self._label

    @property
    def units(self):
        if self._units is None and self._cdim is not None:
            self._units = self._cdim.units()
        return self._units

    def __repr__(self):
        return "<Dim: {0}>".format(self.label) if self.label else "<Dim>"
