from __future__ import (division, print_function, absolute_import)


class Dim(object):

    def __init__(self, cdim):
        self._cdim = cdim

    @property
    def data(self):
        return self._cdim.data()

    @property
    def label(self):
        return self._cdim.label()

    @property
    def units(self):
        return self._cdim.units()

    def __repr__(self):
        return "<Dim: {0}>".format(self.label) if self.label else "<Dim>"
