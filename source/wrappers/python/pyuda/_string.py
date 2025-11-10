from __future__ import (division, print_function, absolute_import)

from ._dim import Dim
from ._utils import cdata_to_numpy_array
from ._data import Data

import copy
import json


class StringDataImpl(Data):
    def __str__(self):
        return self.str

    def plot(self):
        raise NotImplementedError("plot function not implemented for String objects")

    def widget(self):
        raise NotImplementedError("widget function not implemented for String objects")

    def jsonify(self, indent=None):
        obj = {
            'data': {
                '_type': 'string',
                'value': self.str
            },
        }
        return json.dumps(obj, indent=indent)


class String(StringDataImpl):

    def __init__(self, cresult):
        self._cresult = cresult
        self._data = None

    @property
    def str(self):
        if self._data is None:
            self._data = self._cresult.data()
        return self._data

    def clone(self):
        return copy.deepcopy(self)

    def __deepcopy__(self, memo):
        """
        Copy all signal data from cpyuda into a DataOwningString object
        """
        return DataOwningString(str=copy.deepcopy(self.str, memo))

    def __reduce__(self):
        """
        Overwriting __reduce__ method for pickling pyuda string signal objects.
        This does deep copies of all the data views held by a signal object
        and constructs a data owning object which is picklable and can
        be loaded from disk without state initialisation errors in cpyuda.
        """
        return (DataOwningString, (copy.deepcopy(self.str),))


class DataOwningString(StringDataImpl):
    def __init__(self, str=None):
        self.str = str
