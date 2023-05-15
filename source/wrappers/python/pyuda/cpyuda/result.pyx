#cython: language_level=3

cimport uda
cimport numpy as np
from cpython.bytes cimport PyBytes_FromStringAndSize
from cpython.ref cimport Py_INCREF

import numpy

np.import_array()



cdef class Result:

    cdef Handle _handle
    cdef int _is_tree
    cdef dict _meta

    def __init__(self, Handle handle):
        self._handle = handle
        self._is_tree = 1 if uda.setIdamDataTree(handle) != 0 else 0
        cdef uda.SIGNAL_DESC* signal_desc
        cdef uda.DATA_SOURCE* source
        self._meta = {}
        if handle >= 0 and uda.getIdamProperties(handle).get_meta:
            signal_desc = uda.getIdamSignalDesc(handle)
            self._meta["signal_name"] = signal_desc.signal_name
            self._meta["signal_alias"] = signal_desc.signal_alias

            source = uda.getIdamDataSource(handle)
            self._meta["path"] = source.path
            self._meta["filename"] = source.filename
            self._meta["format"] = source.format
            self._meta["exp_number"] = source.exp_number
            self._meta["pass"] = source.pass_
            self._meta["pass_date"] = source.pass_date

    def error_message(self):
        return uda.getIdamErrorMsg(self._handle)

    def error_code(self):
        return uda.getIdamErrorCode(self._handle)

    def rank(self):
        cdef int rank = uda.getIdamRank(self._handle)
        return rank

    cdef int _size(self):
        cdef int size = uda.getIdamDataNum(self._handle)
        return size

    cdef int _type(self, int data_type):
        cdef int type
        if data_type == DataType.DATA:
            type = uda.getIdamDataType(self._handle)
        else:
            type = uda.getIdamErrorType(self._handle)
        return type

    def is_string(self):
        cdef int type = uda.getIdamDataType(self._handle)
        return type == 17

    def is_capnp(self):
        cdef int type = uda.getIdamDataType(self._handle)
        IF CAPNP:
            return type == 22
        ELSE:
            if type == 22:    
                raise NotImplementedError('UDA built without Capn Proto support.')
            return False

    def capnp_tree(self):
        IF CAPNP:
            return CapnpTreeNode.new_(self._handle, NULL, NULL)
        ELSE:
            raise NotImplementedError('UDA built without Capn Proto support.')

    cdef const char* _get_data(self, int data_type):
        cdef const char* data
        if data_type == DataType.DATA:
            data = uda.getIdamData(self._handle)
        else:
            data = uda.getIdamError(self._handle)
        return data

    cdef _data(self, int data_type):
        cdef const char* data = self._get_data(data_type)
        cdef int size
        cdef int type = self._type(data_type)
        cdef int rank = uda.getIdamRank(self._handle)
        cdef int i
        cdef np.npy_intp shape[1024]
        if rank == 0:
            size = self._size()
            shape[0] = <np.npy_intp> size
        else:
            for i in range(rank):
                size = uda.getIdamDimNum(self._handle, rank - 1 - i)
                shape[i] = <np.npy_intp> size
        arr = to_python_i(type, rank, shape, <void *>data)
        if isinstance(arr, np.ndarray):
            np.PyArray_SetBaseObject(arr, self._handle)
            Py_INCREF(self._handle)
        return arr

    def data(self):
        return self._data(DataType.DATA)

    def errors(self):
        return self._data(DataType.ERRORS)

    def label(self):
        return uda.getIdamDataLabel(self._handle).decode(errors='replace') if self._handle >= 0 else ""

    def units(self):
        return uda.getIdamDataUnits(self._handle).decode(errors='replace') if self._handle >= 0 else ""

    def description(self):
        return uda.getIdamDataDesc(self._handle).decode(errors='replace') if self._handle >= 0 else ""

    def shape(self):
        cdef int rank = uda.getIdamRank(self._handle)
        shape = numpy.zeros(rank, dtype=numpy.int32)
        for i in range(rank):
            shape[i] = uda.getIdamDimNum(self._handle, rank - 1 - i)
        return shape

    def dim(self, num, data_type):
        return Dim(self._handle, num, data_type)

    def is_tree(self):
        return bool(self._is_tree)

    def tree(self):
        cdef uda.NTREE* root = uda.getIdamDataTree(self._handle)
        return TreeNode.new_(self._handle, root)

    def meta(self):
        return self._meta

    def has_time_dim(self):
        cdef int order = uda.getIdamOrder(self._handle)
        return order >= 0

    def time_order(self):
        cdef int order = uda.getIdamOrder(self._handle)
        return order

    def time_dim(self, data_type):
        cdef int order = uda.getIdamOrder(self._handle)
        return Dim(self._handle, order, data_type)
