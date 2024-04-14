from __future__ import (division, print_function, absolute_import)

from ._dim import Dim
from ._utils import cdata_to_numpy_array
from ._data import Data

import json


class String(Data):

    def __init__(self, cresult):
        self._cresult = cresult
        self._data = None

    @property
    def str(self):
        if self._data is None:
            self._data = self._cresult.data()
        return self._data

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