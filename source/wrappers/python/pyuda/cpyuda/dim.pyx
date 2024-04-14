#cython: language_level=3

cimport numpy as np
from cpython.bytes cimport PyBytes_FromStringAndSize
from cpython.ref cimport Py_INCREF


np.import_array()


cdef class Dim:
    cdef Handle _handle
    cdef int _num
    cdef int _data_type

    def __init__(self, Handle handle, int num, int data_type):
        self._handle = handle
        self._num = num
        self._data_type = data_type

    def label(self):
        cdef const char* label = uda.getIdamDimLabel(int(self._handle), self._num)
        return label.decode(errors='replace')

    def units(self):
        cdef const char* units = uda.getIdamDimUnits(int(self._handle), self._num)
        return units.decode(errors='replace')

    cdef int _size(self):
        cdef int size = uda.getIdamDimNum(int(self._handle), self._num)
        return size

    cdef int _type(self):
        cdef int type = uda.getIdamDimType(int(self._handle), self._num)
        return type

    def type(self):
        cdef int type
        if self._data_type == DataType.DATA:
            type = uda.getIdamDimType(int(self._handle), self._num)
        else:
            type = uda.getIdamDimErrorType(int(self._handle), self._num)
        return type

    cdef const char* _data(self):
        cdef const char* data
        if self._data_type == DataType.DATA:
            data = uda.getIdamDimData(int(self._handle), self._num)
        else:
            data = uda.getIdamDimError(int(self._handle), self._num)
        return data

    def data(self):
        cdef const char* data = self._data()
        cdef int size = self._size()
        cdef int type = self._type()
        cdef np.npy_intp shape[1]
        shape[0] = <np.npy_intp> size
        cdef int numpy_type = uda_type_to_numpy_type(type)
        arr = np.PyArray_SimpleNewFromData(1, shape, numpy_type, <void*> data)
        np.PyArray_SetBaseObject(arr, self._handle)
        Py_INCREF(self._handle)
        return arr

    def bytes(self):
        cdef const char* data = self._data()
        cdef int size = self._size()
        return PyBytes_FromStringAndSize(data, size)
