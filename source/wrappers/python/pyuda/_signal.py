from __future__ import (division, print_function, absolute_import)

import json
import base64
import numpy as np

import cpyuda

from ._dim import Dim
from ._data import Data


class DimEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, Dim):
            dim = obj
            obj = {
                '_type': 'pyuda.Dim',
                'label': dim.label,
                'units': dim.units,
                'data': {
                    '_encoding': 'base64',
                    '_dtype': dim.data.dtype.name,
                    'value': base64.urlsafe_b64encode(dim.data.tostring()).decode()
                },
            }
            return obj
        return super().default(obj)


class SignalEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, Signal):
            signal = obj
            dim_enc = DimEncoder()
            if len(signal.dims) == 0:
                data = np.array(signal.data)
            else:
                data = signal.data
            obj = {
                '_type': 'pyuda.Signal',
                'label': signal.label,
                'units': signal.units,
                'dims': [dim_enc.default(dim) for dim in signal.dims],
                'data': {
                    '_encoding': 'base64',
                    '_dtype': data.dtype.name,
                    'value': base64.urlsafe_b64encode(data.tostring()).decode()
                },
                'meta': signal.meta,
            }
            return obj
        return super().default(obj)


class Signal(Data):

    def __init__(self, cresult):
        self._cresult = cresult
        self._dims = None
        self._time = None

    @property
    def data(self):
        return self._cresult.data()

    @property
    def errors(self):
        return self._cresult.errors()

    @property
    def label(self):
        return self._cresult.label()

    @property
    def units(self):
        return self._cresult.units()

    @property
    def description(self):
        return self._cresult.description()

    @property
    def rank(self):
        return self._cresult.rank()

    @property
    def shape(self):
        return self._cresult.shape()

    @property
    def dims(self):
        if self._dims is None and self._cresult is not None:
            self._import_dims()
        return self._dims

    @property
    def time(self):
        if self._time is None and self._cresult is not None and self._cresult.has_time_dim():
            self._import_time()
        return self._time

    @property
    def meta(self):
        return self._cresult.meta()

    def _import_dims(self):
        self._dims = []
        for i in range(self._cresult.rank() - 1, -1, -1):
            self._import_dim(i)

    def _import_dim(self, num):
        self._dims.append(Dim(self._cresult.dim(num, cpyuda.DataType.DATA.value)))

    def _import_time(self):
        self._time = Dim(self._cresult.time_dim(cpyuda.DataType.DATA.value))

    def plot(self):
        import matplotlib.pyplot as plt

        dim = self.dims[0]

        plt.plot(dim.data, self.data)
        plt.xlabel('{0} ({1})'.format(dim.label, dim.units))
        plt.ylabel('{0} ({1})'.format(self.label, self.units))
        plt.show()

    def widget(self):
        raise NotImplementedError("widget function not implemented for Signal objects")

    def jsonify(self, indent=None):
        return json.dumps(self, cls=SignalEncoder, indent=indent)

    def __repr__(self):
        return "<Signal: {0}>".format(self.label) if self.label else "<Signal>"

