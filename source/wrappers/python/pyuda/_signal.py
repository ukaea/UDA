from __future__ import (division, print_function, absolute_import)

import json
import base64
import numpy as np
import copy

import cpyuda

from ._dim import Dim
from ._data import Data


class DimEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, np.integer):
            return int(obj)
        elif isinstance(obj, np.floating):
            return float(obj)
        elif isinstance(obj, Dim):
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


class SignalDataImpl(Data):
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


class Signal(SignalDataImpl):

    def __init__(self, cresult):
        self._cresult = cresult
        self._dims = None
        self._time = None
        self._time_index = None
        self._dim_order_reversed = True

    @property
    def data(self):
        if self._dim_order_reversed or self._cresult.rank() <= 1:
            return self._cresult.data()
        elif self._cresult.rank() > 1:
            return np.transpose(self._cresult.data())

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
        if not self._cresult.is_string():
            return self._cresult.rank()
        else:
            return self._cresult.rank() - 1

    @property
    def shape(self):
        if not self._cresult.is_string():
            return tuple(self._cresult.shape())
        else:
            return tuple(self._cresult.shape()[0:-1])

    @property
    def dims(self):
        if self._dims is None and self._cresult is not None:
            self._import_dims()
        return self._dims

    @property
    def time(self):
        if self._time is None and self._cresult is not None and self._cresult.has_time_dim():
            self._import_time()
            self._import_time_index()
        return self._time

    @property
    def time_index(self):
        if self._time_index is None and self._cresult is not None and self._cresult.has_time_dim():
            self._import_time_index()
        return self._time_index

    @property
    def meta(self):
        return self._cresult.meta()

    @property
    def data_block_size(self):
        return self._cresult.data_block_size()

    def _import_dims(self):
        self._dims = []

        if self._dim_order_reversed:
            if not self._cresult.is_string():
                trange = range(self._cresult.rank() - 1, -1, -1)
            else:
                trange = range(self._cresult.rank() - 1, 0, -1)
        else:
            if not self._cresult.is_string():
                trange = range(0, self._cresult.rank()+1)
            else:
                trange = range(1, self._cresult.rank()+1)

        for i in trange:
            self._import_dim(i)

    def _import_dim(self, num):
        self._dims.append(Dim(self._cresult.dim(num, cpyuda.DATA)))

    def _import_time(self):
        self._time = Dim(self._cresult.time_dim(cpyuda.DATA))

    def _import_time_index(self):
        time_order = self._cresult.time_order()

        if self._dim_order_reversed:
            self._time_index = self._cresult.rank() - 1 - time_order
        else:
            self._time_index = time_order

        if self._cresult.is_string():
            self._time_index -= 1

    def reverse_dimension_order(self):
        if self.rank <= 1:
            return

        self._dim_order_reversed = not self._dim_order_reversed

        # If dims and time have already been imported re-order them
        if self._dims is not None:
            self._dims.reverse()

        if self._time_index is not None:
            self._import_time_index()

    def set_time_first(self):
        if self._cresult is None or not self._cresult.has_time_dim():
            return

        if self.rank <= 1:
            return

        if self.time_index > 0 and self.time_index < self.rank - 1:
            raise NotImplementedError("Time is neither first nor last dimension so can not be set as first dimension")

        if self.time_index == self.rank - 1:
            # This will be time last, so reverse dimensions to set time first
            self.reverse_dimension_order()

    def set_time_last(self):
        if self._cresult is None or not self._cresult.has_time_dim():
            return

        if self.rank <= 1:
            return

        if self.time_index > 0 and self.time_index < self.rank - 1:
            raise NotImplementedError("Time is neither first nor last dimension so can not be set as last dimension")

        if self.time_index == 0:
            self.reverse_dimension_order()

    def clone(self):
        """
        Copy all signal data from cpyuda into a DataOwningSignal object
        """
        return copy.deepcopy(self)

    def __deepcopy__(self, memo):
        """
        Copy all signal data from cpyuda into a DataOwningSignal object
        """
        return DataOwningSignal(data=copy.deepcopy(self.data, memo),
                                errors=copy.deepcopy(self.errors, memo),
                                label=copy.deepcopy(self.label, memo),
                                units=copy.deepcopy(self.units, memo),
                                description=copy.deepcopy(self.description, memo),
                                rank=copy.deepcopy(self.rank, memo),
                                dims=copy.deepcopy(self.dims, memo),
                                shape=copy.deepcopy(self.shape, memo),
                                time_index=copy.deepcopy(self.time_index, memo),
                                meta=copy.deepcopy(self.meta, memo))

    def __reduce__(self):
        """
        Overwriting __reduce__ method for pickling pyuda signal objects.
        This does deep copies of all the data views held by a signal object
        and constructs a data owning signal object which is picklable and can
        be loaded from disk without state initialisation errors in cpyuda.
        """
        return (DataOwningSignal, (copy.deepcopy(self.data),
                                   copy.deepcopy(self.errors),
                                   copy.deepcopy(self.label),
                                   copy.deepcopy(self.units),
                                   copy.deepcopy(self.description),
                                   copy.deepcopy(self.rank),
                                   copy.deepcopy(self.dims),
                                   copy.deepcopy(self.shape),
                                   copy.deepcopy(self.time_index),
                                   copy.deepcopy(self.meta)))


class DataOwningSignal(SignalDataImpl):
    """
    Class to hold a copy of a pyuda Signal object where all data arrays are owned
    by the object instead of being views of memory held by the uda c-library
    """

    def __init__(self, data=None, errors=None, label='', units='', description='',
                 rank=None, dims=None, shape=None, time_index=None, meta=None):
        self.data = data
        self.errors = errors
        self.label = label
        self.units = units
        self.descritpion = description
        self.rank = rank
        self.dims = dims
        self.shape = shape
        self.time_index = time_index
        self.meta = meta
        if self.time_index is not None:
            self.time = self.dims[self.time_index]
        else:
            self.time = None
