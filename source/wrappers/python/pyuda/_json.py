from __future__ import (division, print_function, absolute_import)

import json
import base64
import numpy as np

from ._signal import Signal
from ._dim import Dim


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
                    '_type': 'base64',
                    '_dtype': dim.data.dtype.name,
                    'value': base64.urlsafe_b64encode(dim.data.tostring()).decode()
                },
            }
            return obj
        return super().default(obj)


class SignalEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, np.integer):
            return int(obj)
        elif isinstance(obj, np.floating):
            return float(obj)
        elif isinstance(obj, Signal):
            signal = obj
            dim_enc = DimEncoder()
            obj = {
                '_type': 'pyuda.Signal',
                'label': signal.label,
                'units': signal.units,
                'dims': [dim_enc.default(dim) for dim in signal.dims],
                'data': {
                    '_type': 'base64',
                    '_dtype': signal.data.dtype.name,
                    'value': base64.urlsafe_b64encode(signal.data.tostring()).decode()
                },
                'meta': signal.meta,
            }
            return obj
        return super().default(obj)


class SignalDecoder(json.JSONDecoder):

    def __init__(self, *args, **kwargs):
        super().__init__(object_hook=self.object_hook, *args, **kwargs)

    def object_hook(self, obj):
        if '_type' not in obj:
            return obj
        type = obj['_type']
        if type == 'base64':
            return np.frombuffer(base64.urlsafe_b64decode(obj['value']), dtype=obj['_dtype'])
        elif type == 'pyuda.Signal':
            signal = Signal(None)
            signal._data = obj['data']
            signal._dims = obj['dims']
            signal._meta = obj['meta']
            signal._label = obj['label']
            signal._units = obj['units']
            signal._rank = len(obj['dims'])
            return signal
        elif type == 'pyuda.Dim':
            dim = Dim(None)
            dim._data = obj['data']
            dim._label = obj['label']
            dim._units = obj['units']
            return dim
        return obj
