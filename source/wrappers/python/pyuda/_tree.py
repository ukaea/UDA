from __future__ import (division, print_function, absolute_import)

from ._data import Data

import json
import numpy as np
import base64
from typing import List, Dict, Union


class TreeDataEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, np.integer):
            return int(obj)
        elif isinstance(obj, np.floating):
            return float(obj)
        elif isinstance(obj, np.ndarray):
            data = obj
            obj = {
                '_type': 'numpy.ndarray',
                'size': data.size,
                'shape': data.shape,
                'data': {
                    '_encoding': 'base64',
                    '_dtype': data.dtype.name,
                    'value': base64.urlsafe_b64encode(data.tostring()).decode()
                },
            }
            return obj
        return super().default(obj)


class Tree(Data):

    def __init__(self, cnode):
        self._cnode = cnode
        self._children = None
        self._name = self._cnode.name()
        self._data = None

    def _import_data(self):
        self._data = self._cnode.data()

    def _display(self, depth, level):
        if depth is not None and level > depth:
            return
        print(('|' * level + '['), self._name, ']')
        for child in self.children:
            child._display(depth, level+1)
        if self.data is not None:
            print(('|' * (level + 1) + '->'), np.array2string(self._data, threshold=10))

    def display(self, depth=None):
        self._display(depth, 0)

    @property
    def children(self) -> List["Tree"]:
        if self._children is None:
            self._import_children()
        return self._children

    @property
    def data(self):
        if self._data is None:
            self._import_data()
        return self._data

    @property
    def name(self):
        return self._name

    def _import_children(self):
        self._children = []
        for child in self._cnode.children():
            self._children.append(Tree(child))

    def __repr__(self):
        return "<Tree: {0}>".format(self._name)

    def __getitem__(self, item):
        tokens = tuple(i for i in item.split("/") if len(i) > 0)
        if len(tokens) == 0:
            return self
        name = tokens[0]
        found = tuple(c for c in self.children if c.name.lower() == name.lower())
        if len(found) == 0:
            raise KeyError("Cannot find child " + name + " in node " + self.name)
        if len(found) == 1:
            child = found[0]
            return child["/".join(tokens[1:])]
        else:
            return [child["/".join(tokens[1:])] for child in found]

    def plot(self):
        raise NotImplementedError("plot function not implemented for Tree objects")

    def widget(self):
        raise NotImplementedError("widget function not implemented for Tree objects")

    def _todict(self) -> Union[Dict, np.ndarray]:
        if len(self.children) > 0:
            obj = {}
            for child in self.children:
                if child.name in obj:
                    if not isinstance(obj[child.name], list):
                        obj[child.name] = [obj[child.name]]
                    obj[child.name].append(child._todict())
                else:
                    obj[child.name] = child._todict()
            return obj
        else:
            return self.data

    def jsonify(self, indent=None) -> str:
        obj = {self.name: self._todict()}
        return json.dumps(obj, cls=TreeDataEncoder, indent=indent)
