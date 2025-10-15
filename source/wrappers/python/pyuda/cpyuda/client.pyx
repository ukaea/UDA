#cython: language_level=3

import os
import numpy as np
cimport uda
cimport numpy as np
from cpython.version cimport PY_MAJOR_VERSION
from libc cimport string
from libc.stdlib cimport malloc, free
from libc.string cimport strlen


_pid = None
_properties = {
    "get_datadble": ("GET_DATA_DOUBLE", False),
    "get_dimdble": ("GET_DIM_DOUBLE", False),
    "get_timedble": ("GET_TIME_DOUBLE", False),
    "get_bytes": ("GET_BYTES", False),
    "get_bad": ("GET_BAD", False),
    "get_asis": ("GET_AS_IS", False),
    "get_uncal": ("GET_UNCALIBRATED", False),
    "get_notoff": ("GET_NOT_OFFSET", False),
    "get_synthetic": ("GET_SYNTHETIC", False),
    "get_scalar": ("GET_SCALAR", False),
    "get_nodimdata": ("GET_NO_DIM_DATA", False),
    "get_meta": ("META", False),
    "timeout": ("TIMEOUT", True),
    "verbose": ("VERBOSE", False),
    "debug": ("DEBUG", False),
    "altdata": ("ALT_DATA", True),
    "altrank": ("ALT_RANK", False),
    "reuselasthandle": ("REUSE_LAST_HANDLE", False),
    "freeandreuselasthandle": ("FREE_AND_REUSE_LAST_HANDLE", False),
    "filecache": ("FILE_CACHE", False),
}

if PY_MAJOR_VERSION >= 3.0:
    Properties = type('Properties', (), {v[0]:k for k,v in _properties.items()})
else:
    Properties = type(b'Properties', (), {v[0]:k for k,v in _properties.items()})


def set_pid():
    _pid = os.getpid()


def set_property(prop_name, value):
    if prop_name.lower() not in _properties:
        raise ValueError('invalid property ' + prop_name)
    if _properties[prop_name][1]:
        prop_string = prop_name + '=' + str(value)
        uda.udaSetProperty(prop_string.encode())
    elif value:
        uda.udaSetProperty(prop_name.encode())
    else:
        uda.udaResetProperty(prop_name.encode())


def get_property(prop_name):
    if prop_name.lower() not in _properties:
        raise ValueError('invalid property ' + prop_name)
    prop = uda.udaGetProperty(prop_name.encode())
    if _properties[prop_name][1]:
        return prop
    else:
        return bool(prop)


def get_server_host_name():
    return uda.getIdamServerHost().decode()


def get_server_port():
    return uda.getIdamServerPort()


def get_build_version():
    return uda.getUdaBuildVersion()


def get_build_date():
    return uda.getUdaBuildDate()


def set_server_host_name(host_name):
    uda.putIdamServerHost(host_name.encode())


def set_server_port(port):
    uda.putIdamServerPort(port)


def close_connection():
    uda.closeAllConnections()


def get_data(signal, source):
    if _pid != os.getpid():
        raise ClientException("Calling client from a process different to process in which library was initialised")
    handle = uda.idamGetAPI(signal.encode(), source.encode())
    cdef const char* err_msg
    cdef int err_code
    if handle < 0:
        err_msg = uda.getIdamErrorMsg(handle)
        err_code = uda.getIdamErrorCode(handle)
        if err_msg == NULL or string.strlen(err_msg) == 0:
            raise UDAException("unknown error occured")
        elif err_code < 0:
            raise ClientException(err_msg.decode())
        else:
            raise ServerException(err_msg.decode())
    return Result(Handle(handle))


def get_data_batch(signals, sources):
    if _pid != os.getpid():
        raise ClientException("Calling client from a process different to process in which library was initialised")
    assert len(signals) == len(sources)
    cdef const char** signals_array = <const char**>malloc(len(signals) * sizeof(char*))
    cdef const char** sources_array = <const char**>malloc(len(sources) * sizeof(char*))
    cdef int* handles = <int*>malloc(len(signals) * sizeof(int))
    signal_bytes = []
    source_bytes = []
    try:
        for i, signal in enumerate(signals):
            bytes = signal.encode()
            signal_bytes.append(bytes)
            signals_array[i] = bytes
        for i, source in enumerate(sources):
            bytes = source.encode()
            source_bytes.append(bytes)
            sources_array[i] = bytes
        rc = uda.idamGetBatchAPI(signals_array, sources_array, len(signals), handles)
        if rc < 0:
            err_msg = uda.getIdamErrorMsg(rc)
            err_code = uda.getIdamErrorCode(rc)
            if err_msg == NULL or string.strlen(err_msg) == 0:
                raise UDAException("unknown error occured")
            elif err_code < 0:
                raise ClientException(err_msg.decode())
            else:
                raise ServerException(err_msg.decode())
        results = []
        for i in range(len(signals)):
            results.append(Result(Handle(handles[i])))
        return results
    finally:
        free(signals_array)
        free(sources_array)
        free(handles)


cdef put_nothing(const char* instruction):
    cdef int handle = uda.idamPutAPI(instruction, NULL)
    return Result(Handle(handle))


cdef int UDA_TYPE_UNKNOWN = 0,
cdef int UDA_TYPE_CHAR = 1,
cdef int UDA_TYPE_SHORT = 2,
cdef int UDA_TYPE_INT = 3,
cdef int UDA_TYPE_UNSIGNED_INT = 4,
cdef int UDA_TYPE_LONG = 5,
cdef int UDA_TYPE_FLOAT = 6,
cdef int UDA_TYPE_DOUBLE = 7,
cdef int UDA_TYPE_UNSIGNED_CHAR = 8,
cdef int UDA_TYPE_UNSIGNED_SHORT = 9,
cdef int UDA_TYPE_UNSIGNED_LONG = 10,
cdef int UDA_TYPE_LONG64 = 11,
cdef int UDA_TYPE_UNSIGNED_LONG64 = 12,
cdef int UDA_TYPE_COMPLEX = 13,
cdef int UDA_TYPE_DCOMPLEX = 14,
cdef int UDA_TYPE_UNDEFINED = 15,
cdef int UDA_TYPE_VLEN = 16,
cdef int UDA_TYPE_STRING = 17,
cdef int UDA_TYPE_COMPOUND = 18,
cdef int UDA_TYPE_OPAQUE = 19,
cdef int UDA_TYPE_ENUM = 20,
cdef int UDA_TYPE_VOID  = 21,
cdef int UDA_TYPE_STRING2 = 99

numpy2uda_type_map = {
    np.NPY_BOOL: UDA_TYPE_INT,
    np.NPY_BYTE: UDA_TYPE_CHAR,
    np.NPY_UBYTE: UDA_TYPE_UNSIGNED_CHAR,
    np.NPY_SHORT: UDA_TYPE_SHORT,
    np.NPY_USHORT: UDA_TYPE_UNSIGNED_SHORT,
    np.NPY_INT: UDA_TYPE_INT,
    np.NPY_UINT: UDA_TYPE_UNSIGNED_INT,
    np.NPY_LONG: UDA_TYPE_LONG,
    np.NPY_ULONG: UDA_TYPE_UNSIGNED_LONG,
    np.NPY_LONGLONG: UDA_TYPE_LONG64,
    np.NPY_ULONGLONG: UDA_TYPE_LONG64,
    np.NPY_FLOAT: UDA_TYPE_FLOAT,
    np.NPY_DOUBLE: UDA_TYPE_DOUBLE,
    np.NPY_LONGDOUBLE: UDA_TYPE_UNKNOWN,
    np.NPY_CFLOAT: UDA_TYPE_COMPLEX,
    np.NPY_CDOUBLE: UDA_TYPE_DCOMPLEX,
    np.NPY_CLONGDOUBLE: UDA_TYPE_UNKNOWN,
    np.NPY_OBJECT: UDA_TYPE_UNKNOWN,
    np.NPY_STRING: UDA_TYPE_STRING,
    np.NPY_UNICODE: UDA_TYPE_STRING,
    np.NPY_VOID: UDA_TYPE_UNKNOWN,
}


cdef numpy_type_to_UDA_type(int type):
    return numpy2uda_type_map[type]

cdef put_ndarray_string(const char* instruction, np.ndarray data):
    cdef int rank = np.PyArray_NDIM(data)

    if rank > 1:
        raise UDAException("String arrays with more than 1 dimension are not supported for putting to the server")

    cdef uda.PUTDATA_BLOCK put_data
    uda.initIdamPutDataBlock(&put_data)

    cdef np.npy_intp* shape = np.PyArray_DIMS(data)
    cdef int size = data.dtype.itemsize
    cdef int max_str_len = len(max(data, key=len))

    put_data.data_type = UDA_TYPE_STRING
    put_data.rank = rank + 1
    put_data.count = np.PyArray_SIZE(data) * (max_str_len + 1)
    put_data.shape = <int *> malloc((rank + 1) * sizeof(int))

    cdef int i = 0
    while i < rank:
        put_data.shape[i] = shape[i]
        i += 1
    put_data.shape[rank] = max_str_len

    cdef np.ndarray fixed_len_array = np.zeros(np.PyArray_SIZE(data), dtype='S'+str(max_str_len+1))
    for sind, s in enumerate(data):
        fixed_len_array[sind] = s

    put_string = bytearray(fixed_len_array).decode().encode()
    put_data.data = put_string

    cdef int handle = uda.idamPutAPI(instruction, &put_data)
    free(put_data.shape)
    return Result(Handle(handle))


cdef put_ndarray(const char* instruction, np.ndarray data):
    cdef uda.PUTDATA_BLOCK put_data
    uda.initIdamPutDataBlock(&put_data)

    cdef int rank = np.PyArray_NDIM(data)
    cdef np.npy_intp* shape = np.PyArray_DIMS(data)
    cdef int size = data.dtype.itemsize

    put_data.data_type = numpy_type_to_UDA_type(np.PyArray_TYPE(data))
    put_data.rank = rank
    put_data.count = np.PyArray_SIZE(data)
    put_data.shape = <int *> malloc(rank * size)
    cdef int i = 0
    while i < rank:
        put_data.shape[i] = shape[i]
        i += 1
    put_data.data = np.PyArray_BYTES(data)

    cdef int handle = uda.idamPutAPI(instruction, &put_data)
    free(put_data.shape)
    return Result(Handle(handle))


cdef put_scalar(const char* instruction, object data):
    cdef uda.PUTDATA_BLOCK put_data
    uda.initIdamPutDataBlock(&put_data)

    cdef np.dtype type = np.PyArray_DescrFromScalar(data)
    cdef char* bytes = <char *> malloc(type.itemsize)
    np.PyArray_ScalarAsCtype(data, bytes)

    put_data.data_type = numpy_type_to_UDA_type(type.type_num)
    put_data.rank = 0
    put_data.count = 1
    put_data.shape = NULL
    put_data.data = bytes

    cdef int handle = uda.idamPutAPI(instruction, &put_data)
    free(bytes)
    return Result(Handle(handle))


cdef put_string(const char* instruction, const char* data):
    cdef uda.PUTDATA_BLOCK put_data
    uda.initIdamPutDataBlock(&put_data)

    cdef int string_length = strlen(data)

    put_data.data_type = 17 # UDA_TYPE_STRING
    put_data.rank = 0
    put_data.count = string_length + 1
    put_data.shape = NULL
    put_data.data = data

    cdef int handle = uda.idamPutAPI(instruction, &put_data)
    return Result(Handle(handle))


def put_data(instruction, data=None):
    if _pid != os.getpid():
        raise ClientException("Calling client from a process different to process in which library was initialised")
    if isinstance(data, np.ndarray):
        if np.PyArray_TYPE(data) not in (np.NPY_STRING, np.NPY_UNICODE):
            return put_ndarray(instruction, data)
        else:
            return put_ndarray_string(instruction, data)
    elif np.PyArray_CheckScalar(data):
        return put_scalar(instruction, data)
    elif isinstance(data, bytes):
        return put_string(instruction, data)
    else:
        return put_nothing(instruction)

