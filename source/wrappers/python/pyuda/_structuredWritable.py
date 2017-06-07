"""
Extension of StructuredData class, with additional
methods for adding/removing attributes & children.
"""
from __future__ import absolute_import

from ._structured import StructuredData


class StructuredWritable(StructuredData):

    def __init__(self, cnode):
        StructuredData.__init__(self, cnode)

    def add_attr(self, name, data):
        attr_name = self._check_name(name)
        self._imported_attrs.append(attr_name)
        setattr(self, attr_name, data)

    def delete_attr(self, name):
        if name in self._imported_attrs:
            ind_del = self._imported_attrs.index(name)
            delattr(self, name)
            del self._imported_attrs[ind_del]

    @property
    def children(self):
        if self._children is None:
            self._import_children()
        return self._children

    def add_child(self, new_child):
        child_names = []
        # noinspection PyTypeChecker
        for child in self.children:
            child_names.append(child.name)

        if new_child.name in child_names:
            # noinspection PyProtectedMember
            new_child._name += "_"

        self.children.append(new_child)

    def retain_children(self, indices=None):
        if type(indices) is not list and type(indices) is not tuple:
            return

        new_children = [self.children[ind] for ind in indices]
        self._children = new_children

    # noinspection PyTypeChecker
    def delete_child(self, child_name=None, index=None):
        if child_name is None and index is None:
            return

        if child_name is not None:
            child_names = [child.name for child in self.children]
            if child_name in child_names:
                ind_del = child_names.index(child_name)
                del self.children[ind_del]
        elif index < len(self.children):
            del self.children[index]

    # noinspection PyTypeChecker
    def change_child_name(self, old_name, new_name):
        for child in self.children:
            if child._name == old_name:
                child._name = new_name

    # noinspection PyTypeChecker
    def delete_level(self, name_to_delete):
        child_names = [child.name for child in self.children]

        if name_to_delete in child_names:
            ind_del = child_names.index(name_to_delete)
            # noinspection PyProtectedMember
            attrs_names = self.children[ind_del]._imported_attrs
            attrs = [getattr(self.children[ind_del], a_name) for a_name in attrs_names]

            # Add children and attributes back in
            for child in self.children[ind_del].children:
                self.add_child(child)

            for a_name, attr in zip(attrs_names, attrs):
                self.add_attr(a_name, attr)

            child_names = [child.name for child in self.children]
            ind_del = child_names.index(name_to_delete)
            del self._children[ind_del]

    def _import_children(self):
        self._children = []
        for i in range(0, self._cnode.numChildren()):
            self._children.append(StructuredWritable(self._cnode.child(i)))

    def plot(self):
        raise NotImplementedError("plot function not implemented for StructuredWritable objects")

    def widget(self):
        raise NotImplementedError("widget function not implemented for StructuredWritable objects")
