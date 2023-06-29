# cython: language_level=3
"""
Handle class, a thin wrapper around a handle integer that is refcounted.
"""
cimport uda


cdef class Handle(int):
    cdef int _handle

    def __init__(self, handle):
        self._handle = handle

    def __int__(self):
        return self._handle

    def __dealloc__(self):
        uda.udaFree(self._handle)
