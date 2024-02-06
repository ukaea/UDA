include "handle.pyx"

cimport uda
cimport capnp

from libc cimport string
cimport numpy as np
from libc.stdlib cimport malloc, free
from cpython.ref cimport Py_INCREF
from types import uda_type_to_numpy_type


np.import_array()


cdef class CapnpTreeNode:

    cdef Handle _handle
    cdef capnp.TreeReader* _tree
    cdef capnp.NodeReader* _node
    cdef list _children
    cdef int _children_init
    cdef int _data_init
    cdef object _data

    def __init__(self):
        self._children_init = 0
        self._children = []
        self._data_init = 0
        self._data = None

    @staticmethod
    cdef new_(Handle handle, capnp.TreeReader* tree, capnp.NodeReader* node):
        cdef char* data
        cdef int num
        tree_node = CapnpTreeNode()
        tree_node._handle = handle
        if tree == NULL:
            data = uda.udaGetData(int(handle))
            num = uda.udaGetDataNum(int(handle))
            tree_node._tree = capnp.uda_capnp_deserialise(data, num)
            tree_node._node = capnp.uda_capnp_read_root(tree_node._tree)
        else:
            tree_node._tree = tree
            tree_node._node = node
        return tree_node

    def _load_children(self):
        cdef size_t num_children = capnp.uda_capnp_num_children(self._node)
        cdef capnp.NodeReader* child
        for i in range(num_children):
            child = capnp.uda_capnp_read_child_n(self._tree, self._node, i)
            self._children.append(CapnpTreeNode.new_(self._handle, self._tree, child))

    def children(self):
        if not self._children_init:
            self._load_children()
            self._children_init = 1
        return self._children

    def name(self):
        cdef const char* name = capnp.uda_capnp_read_name(self._node)
        return name.decode() if name is not NULL else ""

    cdef _import_data(self):
        cdef capnp.Optional_Size_t maybe_rank = capnp.uda_capnp_read_rank(self._node)
        if not maybe_rank.has_value:
            return

        cdef int type = capnp.uda_capnp_read_type(self._node)
        cdef size_t rank = maybe_rank.value
        cdef size_t* shape = <size_t*>malloc(rank * sizeof(size_t))
        capnp.uda_capnp_read_shape(self._node, shape)

        cdef np.npy_intp* np_shape = <np.npy_intp*>malloc(rank * sizeof(np.npy_intp))
        cdef size_t size = 1;
        for i in range(0, rank):
            size *= shape[i]
            np_shape[i] = shape[i]

        free(shape)

        cdef int np_type = uda_type_to_numpy_type(type)
        self._data = np.PyArray_SimpleNew(<int>rank, np_shape, np_type)
        np.PyArray_SetBaseObject(self._data, self._handle)
        Py_INCREF(self._handle)

        free(np_shape)

        cdef size_t num_slices = capnp.uda_capnp_read_num_slices(self._node)
        cdef char* bytes = NULL
        cdef size_t slice_size = 0
        cdef size_t offset = 0

        if rank == 0:
            if num_slices > 1:
                raise Exception('Invalid scalar data')
            bytes = np.PyArray_BYTES(self._data)
            capnp.uda_capnp_read_data(self._node, 0, bytes)
        else:
            for i in range(0, num_slices):
                bytes = np.PyArray_BYTES(self._data)
                slice_size = capnp.uda_capnp_read_slice_size(self._node, i)
                capnp.uda_capnp_read_data(self._node, i, bytes + offset)
                offset += slice_size

    def data(self):
        if not self._data_init:
            self._import_data()
            self._data_init = 1
        return self._data
