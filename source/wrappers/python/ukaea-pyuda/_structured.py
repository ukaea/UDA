from __future__ import (division, print_function, absolute_import)

from ._utils import (cdata_scalar_to_value, cdata_vector_to_value, cdata_array_to_value)
from ._data import Data

import json
import itertools
import numpy as np
import base64


class StructuredDataEncoder(json.JSONEncoder):

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


class StructuredData(Data):

    _translation_table = None

    def __init__(self, cnode):
        self._cnode = cnode
        self._children = None
        self._name = self._cnode.name() or 'ROOT'
        self._imported_attrs = []
        if StructuredData._translation_table is None:
            StructuredData._setup_translation_table()
        self._import_data()

    @classmethod
    def _setup_translation_table(cls):
        cls._translation_table = ''.join(chr(i) for i in range(256))
        identifier_chars = tuple(itertools.chain(range(ord('0'), ord('9')+1),
                                                 range(ord('a'), ord('z')+1),
                                                 range(ord('A'), ord('Z')+1)))
        identifier_chars += ('_',)
        cls._translation_table = u''.join(c if ord(c) in identifier_chars else '_' for c in cls._translation_table)

    def _import_data(self):
        data = self._cnode.data()
        for (name, value) in data.items():
            if value is not None:
                attr_name = self._check_name(name)
                self._imported_attrs.append(attr_name)
                setattr(self, attr_name, value)

    def _display(self, depth, level):
        if depth is not None and level > depth:
            return
        print(('|' * level + '['), self._name, ']')
        for name in self._imported_attrs:
            print(('|' * (level + 1) + '->'), name)
        for child in self.children:
            child._display(depth, level+1)

    def display(self, depth=None):
        self._display(depth, 0)

    def __getitem__(self, item):
        tokens = tuple(i for i in item.split("/") if len(i) > 0)
        if len(tokens) == 0:
            return self
        if tokens[0] == "ROOT":
            tokens = tokens[1:]
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

    @property
    def children(self):
        if self._children is None:
            self._import_children()
        return self._children

    @property
    def name(self):
        return self._name

    def _import_children(self):
        self._children = []
        for child in self._cnode.children():
            self._children.append(StructuredData(child))

    def _check_name(self, name):
        name = name.translate(self._translation_table)
        attrs = tuple(i for i in dir(self) if not i.startswith('_'))
        keywords = ('and', 'as', 'assert', 'break', 'class', 'continue', 'def', 'del', 'elif',
                    'else', 'except', 'exec', 'finally', 'for', 'from', 'global', 'if', 'import',
                    'in', 'is', 'lambda', 'not', 'or', 'pass', 'print', 'raise', 'return', 'try',
                    'while', 'with', 'yield')
        while name in attrs or name in keywords:
            name += '_'
        return name

    def __repr__(self):
        return "<Structured Data: {0}>".format(self._name)

    def plot(self):
        raise NotImplementedError("plot function not implemented for StructuredData objects")

    def widget(self):
        raise NotImplementedError("widget function not implemented for StructuredData objects")

    def _todict(self):
        obj = {}
        for name in self._imported_attrs:
            obj[name] = getattr(self, name)
        if len(self.children) > 0:
            obj['children'] = []
            for child in self.children:
                obj['children'].append(child._todict())
        return obj

    def jsonify(self, indent=None):
        obj = self._todict()
        return json.dumps(obj, cls=StructuredDataEncoder, indent=indent)
