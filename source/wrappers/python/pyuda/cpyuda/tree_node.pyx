cimport uda

from libc cimport string
cimport numpy as np


np.import_array()


cdef class TreeNode:

    cdef Handle _handle
    cdef uda.NTREE* _node
    cdef list _children
    cdef int _children_init
    cdef dict _values
    cdef int _values_init

    def __init__(self):
        self._children_init = 0
        self._children = []
        self._values_init = 0
        self._values = {}

    @staticmethod
    cdef new_(Handle handle, uda.NTREE* node):
        tree_node = TreeNode()
        tree_node._handle = handle
        tree_node._node = node
        return tree_node

    def _load_children(self):
        cdef int num_children = uda.udaGetNodeChildrenCount(self._node)
        cdef uda.NTREE* child
        for i in range(num_children):
            child = uda.udaGetNodeChild(self._node, i)
            self._children.append(TreeNode.new_(self._handle, child))

    def children(self):
        if not self._children_init:
            self._load_children()
            self._children_init = 1
        return self._children

    def name(self):
        cdef const char* name = uda.udaGetNodeStructureName(self._node)
        return name.decode() if name is not NULL else ""

    cdef _load_atomic_data(self, int idx):
        cdef const char** anames = <const char**>uda.udaGetNodeAtomicNames(self._node)
        cdef const char** atypes = <const char**>uda.udaGetNodeAtomicTypes(self._node)
        cdef int* apoint = uda.udaGetNodeAtomicPointers(self._node)
        cdef int* arank = uda.udaGetNodeAtomicRank(self._node)
        cdef int** ashape = uda.udaGetNodeAtomicShape(self._node)

        cdef const char* name = anames[idx]
        cdef const char* type = atypes[idx]
        cdef int point = apoint[idx]
        cdef int rank = arank[idx]
        cdef int* shape = ashape[idx]

        cdef void* data = uda.udaGetNodeStructureComponentData(self._node, name)
        return to_python_c(type, rank, shape, point, data, <PyObject*>self._handle)

    cdef _import_data(self):
        cdef const char** anames = <const char**>uda.udaGetNodeAtomicNames(self._node)

        cdef int size = uda.udaGetNodeAtomicCount(self._node)
        cdef int i
        cdef const char* name

        for i in range(size):
            data = self._load_atomic_data(i)
            name = anames[i]
            self._values[name.decode()] = data

    def data(self):
        if not self._values_init:
            self._import_data()
            self._values_init = 1
        return self._values
