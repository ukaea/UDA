from ._dim import Dim
from ._utils import cdata_to_numpy_array, cdata_scalar_to_value
from ._data import Data

import json
import base64
import numpy as np


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
        self._data = None
        self._dims = None
        self._meta = None
        self._label = None
        self._units = None
        self._rank = None

    def _import_data(self):
        data = self._cresult.data()
        if not data.isNull():
            if data.size() == 0:
                self._data = cdata_scalar_to_value(data)
            else:
                self._data = cdata_to_numpy_array(data)
                shape = [d.data.size for d in self.dims]
                self._data = self._data.reshape(*shape)

    @property
    def data(self):
        if self._data is None and self._cresult is not None:
            self._import_data()
        return self._data

    @property
    def label(self):
        if self._label is None and self._cresult is not None:
            self._label = self._cresult.label()
        return self._label

    @property
    def units(self):
        if self._units is None and self._cresult is not None:
            self._units = self._cresult.units()
        return self._units

    @property
    def rank(self):
        if self._rank is None and self._cresult is not None:
            self._rank = self._cresult.rank()
        return self._rank

    @property
    def dims(self):
        if self._dims is None and self._cresult is not None:
            self._import_dims()
        return self._dims

    @property
    def meta(self):
        if self._meta is None and self._cresult is not None:
            self._meta = {}
            m = self._cresult.meta()
            for k in m:
                self._meta[k] = m[k]
        return self._meta

    def _import_dims(self):
        self._dims = []
        for i in range(self._cresult.rank() - 1, -1, -1):
            self._import_dim(i)

    def _import_dim(self, num):
        self._dims.append(Dim(self._cresult.dim(num)))

    def plot(self):
        import matplotlib.pyplot as plt

        dim = self.dims[0]

        plt.plot(dim.data, self.data)
        plt.xlabel('{0} ({1})'.format(dim.label, dim.units))
        plt.ylabel('{0} ({1})'.format(self.label, self.units))
        plt.show()

    def widget(self):
        raise NotImplementedError("widget function not implemented for Signal objects")

    def jsonify(self):
        return json.dumps(self, cls=SignalEncoder)

    def __repr__(self):
        return "<Signal: {0}>".format(self.label) if self.label else "<Signal>"

