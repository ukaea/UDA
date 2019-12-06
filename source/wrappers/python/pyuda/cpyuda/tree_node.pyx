cimport uda

from libc cimport string
cimport numpy as np

np.import_array()


cdef class TreeNode:

    cdef int _handle
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
    cdef new(int handle, uda.NTREE* node):
        tree_node = TreeNode()
        tree_node._handle = handle
        tree_node._node = node
        return tree_node

    def _load_children(self):
        cdef int num_children = uda.getNodeChildrenCount(self._node)
        cdef uda.NTREE* child
        for i in range(num_children):
            child = uda.getNodeChild(self._node, i)
            self._children.append(TreeNode.new(self._handle, child))

    def children(self):
        if not self._children_init:
            self._load_children()
            self._children_init = 1
        return self._children

    def name(self):
        cdef const char* name = uda.getNodeStructureName(self._node)
        return name.decode() if name is not NULL else ""

    cdef _load_atomic_data(self, int idx, uda.LOGMALLOCLIST* logmalloclist):
        cdef const char** anames = uda.getNodeAtomicNames(logmalloclist, self._node)
        cdef const char** atypes = uda.getNodeAtomicTypes(logmalloclist, self._node)
        cdef int* apoint = uda.getNodeAtomicPointers(logmalloclist, self._node)
        cdef int* arank = uda.getNodeAtomicRank(logmalloclist, self._node)
        cdef int** ashape = uda.getNodeAtomicShape(logmalloclist, self._node)

        cdef const char* name = anames[idx]
        cdef const char* type = atypes[idx]
        cdef int point = apoint[idx]
        cdef int rank = arank[idx]
        cdef int* shape = ashape[idx]

        cdef void* data = uda.getNodeStructureComponentData(logmalloclist, self._node, name)

        cdef np.npy_intp np_shape[1024]
        cdef int i
        for i in range(rank):
            np_shape[i] = <np.npy_intp>shape[i]

        cdef int np_type
        if string.strstr(type, "STRING"):
            # if type.startswith("STRING"):
            return (<char*>data).decode()
        else:
            np_type = uda_field_type_to_numpy_type(type.decode())
            if np_type >= 0:
                return np.PyArray_SimpleNewFromData(rank, np_shape, np_type, data)
            else:
                return None

    cdef _import_data(self):
        cdef uda.LOGMALLOCLIST* logmalloclist = uda.getIdamLogMallocList(self._handle)
        cdef const char** anames = uda.getNodeAtomicNames(logmalloclist, self._node)

        cdef int size = uda.getNodeAtomicCount(self._node)
        cdef int i
        cdef const char* name

        for i in range(size):
            data = self._load_atomic_data(i, logmalloclist)
            name = anames[i]
            self._values[name.decode()] = data

    def data(self):
        if not self._values_init:
            self._import_data()
            self._values_init = 1
        return self._values
