import itertools
from ._utils import cdata_scalar_to_value, cdata_vector_to_value
from ._data import Data


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
        cls._translation_table = ''.join(c if ord(c) in identifier_chars else '_' for c in cls._translation_table)

    def _import_data(self):
        ptrs = self._cnode.atomicPointers()
        ranks = self._cnode.atomicRank()
        types = self._cnode.atomicTypes()
        for (i, name) in enumerate(self._cnode.atomicNames()):
            value = None
            if types[i] == 'STRING *' and (ranks[i] == 1 or ptrs[i]):
                vector = self._cnode.atomicVector(name)
                if not vector.isNull():
                    value = cdata_vector_to_value(vector)
            else:
                scalar = self._cnode.atomicScalar(name)
                if not scalar.isNull():
                    value = cdata_scalar_to_value(scalar)
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
        index = None
        if "@" in name:
            (name, index) = name.split("@")
        found = tuple(c for c in self.children if c.name == name)
        if len(found) == 0:
            raise KeyError("Cannot find child " + name + " in node " + self.name)
        if index is None:
            if len(found) > 1:
                raise KeyError("Multiple children found with name " + name + " in node " + self.name)
            child = found[0]
        else:
            if not (0 <= int(index) < len(found)):
                print(len(found))
                raise IndexError("Index " + index + " out of range for child " + name + " in node " + self.name)
            child = found[int(index)]
        return child["/".join(tokens[1:])]

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
        for i in range(0, self._cnode.numChildren()):
            self._children.append(StructuredData(self._cnode.child(i)))

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