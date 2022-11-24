
from cpython.ref cimport PyObject
from cpython.ref cimport Py_INCREF

__uda2np_map = {
    1: np.NPY_INT8, # UDA_TYPE_CHAR = 1,
    2: np.NPY_INT16, # UDA_TYPE_SHORT = 2,
    3: np.NPY_INT32, # UDA_TYPE_INT = 3,
    4: np.NPY_UINT32, # UDA_TYPE_UNSIGNED_INT = 4,
    5: np.NPY_INT64, # UDA_TYPE_LONG = 5,
    6: np.NPY_FLOAT32, # UDA_TYPE_FLOAT = 6,
    7: np.NPY_FLOAT64, # UDA_TYPE_DOUBLE = 7,
    8: np.NPY_UINT8, # UDA_TYPE_UNSIGNED_CHAR = 8,
    9: np.NPY_UINT16, # UDA_TYPE_UNSIGNED_SHORT = 9,
    10: np.NPY_UINT64, # UDA_TYPE_UNSIGNED_LONG = 10,
    11: np.NPY_INT64, # UDA_TYPE_LONG64 = 11,
    12: np.NPY_UINT64, # UDA_TYPE_UNSIGNED_LONG64 = 12,
    13: np.NPY_COMPLEX64, # UDA_TYPE_COMPLEX = 13,
    14: np.NPY_COMPLEX128, # UDA_TYPE_DCOMPLEX = 14,
}

__np2uda_map = dict((__uda2np_map[i], i) for i in __uda2np_map)

__field_types = {
    "char": np.NPY_INT8,                    # UDA_TYPE_CHAR = 1,
    "short": np.NPY_INT16,                  # UDA_TYPE_SHORT = 2,
    "int": np.NPY_INT32,                    # UDA_TYPE_INT = 3,
    "unsigned int": np.NPY_UINT32,          # UDA_TYPE_UNSIGNED_INT = 4,
    "long": np.NPY_INT64,                   # UDA_TYPE_LONG = 5,
    "float": np.NPY_FLOAT32,                # UDA_TYPE_FLOAT = 6,
    "double": np.NPY_FLOAT64,               # UDA_TYPE_DOUBLE = 7,
    "unsigned char": np.NPY_UINT8,          # UDA_TYPE_UNSIGNED_CHAR = 8,
    "unsigned short": np.NPY_UINT16,        # UDA_TYPE_UNSIGNED_SHORT = 9,
    "unsigned long": np.NPY_UINT64,         # UDA_TYPE_UNSIGNED_LONG = 10,
    "long long": np.NPY_INT64,              # UDA_TYPE_LONG64 = 11,
    "unsigned long long": np.NPY_UINT64,    # UDA_TYPE_UNSIGNED_LONG64 = 12,
}


cdef int uda_type_to_numpy_type(int type):
    return __uda2np_map.get(type, -1)


cdef int numpy_type_to_uda_type(int type):
    return __np2uda_map.get(type, -1)


cdef int uda_field_type_to_numpy_type(str type):
    base_type = type.replace(" *", "")
    return __field_types.get(base_type, -1)


cdef object to_python_c(const char* type, int rank, int* shape, int point, void* data, PyObject* base):
    cdef np.npy_intp np_shape[1024]
    cdef int i
    for i in range(rank):
        np_shape[i] = <np.npy_intp>shape[rank - 1 - i]

    cdef int np_type
    strings = []
    if string.strstr(type, "STRING"):
        if string.strcmp(type, "STRING") == 0:
            return (<char*>data).decode()
        elif rank == 0 and shape[0] == 1 and point == 0:
            return (<char**>data)[0].decode()
        else:
            for i in range(shape[0]):
                strings.append((<char**>data)[i].decode())
            return strings
    else:
        np_type = uda_field_type_to_numpy_type(type.decode())
        if np_type >= 0:
            Py_INCREF(<object>base)
            if point and rank == 0:
                np_shape[0] = shape[0]
                arr = np.PyArray_SimpleNewFromData(1, np_shape, np_type, data)
                np.pyArray_SetBaseObject(arr, <object>base)
                return arr
            else:
                arr = np.PyArray_SimpleNewFromData(rank, np_shape, np_type, data)
                np.pyArray_SetBaseObject(arr, <object>base)
                if rank == 0:
                    return arr.sum()
                else:
                    return arr
        else:
            return None


cdef object to_python_i(int type, int rank, np.npy_intp* np_shape, void* data):
    cdef int np_type
    cdef np.npy_intp shape[1024]

    if type == 17 and rank <= 1:
        return (<char*>data).decode()
    elif type == 17 and rank > 1:
        slen = np_shape[rank-1]
        for i in range(rank):
            shape[i] = <np.npy_intp> np_shape[i] 

        arr = np.PyArray_SimpleNewFromData(rank, shape, np.NPY_BYTE, data)
        arr_bytes = bytearray(arr).decode()

        return np.array([arr_bytes[s:s+slen] for s in range(0, len(arr_bytes), slen)], dtype='U'+repr(slen))
    else:
        np_type = uda_type_to_numpy_type(type)
        if np_type >= 0:
            arr = np.PyArray_SimpleNewFromData(rank, np_shape, np_type, data)
            if rank == 0:
                return arr.sum()
            else:
                return arr
        else:
            return None


ctypedef public enum DataType:
    DATA = 0
    ERRORS = 1
