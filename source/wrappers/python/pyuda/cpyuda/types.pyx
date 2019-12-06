__types = {
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

__field_types = {
    "char": np.NPY_INT8, # UDA_TYPE_CHAR = 1,
    "short": np.NPY_INT16, # UDA_TYPE_SHORT = 2,
    "int": np.NPY_INT32, # UDA_TYPE_INT = 3,
    "unsigned int": np.NPY_UINT32, # UDA_TYPE_UNSIGNED_INT = 4,
    "long": np.NPY_INT64, # UDA_TYPE_LONG = 5,
    "float": np.NPY_FLOAT32, # UDA_TYPE_FLOAT = 6,
    "double": np.NPY_FLOAT64, # UDA_TYPE_DOUBLE = 7,
    "unsigned char": np.NPY_UINT8, # UDA_TYPE_UNSIGNED_CHAR = 8,
    "unsigned short": np.NPY_UINT16, # UDA_TYPE_UNSIGNED_SHORT = 9,
    "unsigned long": np.NPY_UINT64, # UDA_TYPE_UNSIGNED_LONG = 10,
}

print(np.NPY_INT16)

cdef int uda_type_to_numpy_type(int type):
    return __types.get(type, -1)


cdef int uda_field_type_to_numpy_type(str type):
    base_type = type.replace(" *", "")
    return __field_types.get(base_type, -1)


from enum import Enum


class DataType(Enum):
    DATA = 0
    ERRORS = 1
