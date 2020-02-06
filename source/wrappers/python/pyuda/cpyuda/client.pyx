#cython: language_level=3

cimport uda
cimport numpy as np
from libc cimport string
from libc.stdlib cimport malloc, free
from libc.string cimport strlen


def set_property(prop_name, value):
    if prop_name in ('timeout', 'altRank'):
        prop_string = prop_name + '=' + str(value)
    else:
        prop_string = prop_name
    uda.setIdamProperty(prop_string.encode())


def get_property(prop_name):
    prop = uda.getIdamProperty(prop_name).decode()
    if prop_name in ('timeout', 'altRank'):
        return prop
    else:
        return bool(prop)


def get_server_host_name():
    return uda.getIdamServerHost().decode()


def get_server_port():
    return uda.getIdamServerPort()


def set_server_host_name(host_name):
    uda.putIdamServerHost(host_name.encode())


def set_server_port(port):
    uda.putIdamServerPort(port)


def get_data(signal, source):
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
    return Result(handle)


cdef put_ndarray(const char* instruction, np.ndarray data):
    cdef uda.PUTDATA_BLOCK put_data
    uda.initIdamPutDataBlock(&put_data)

    cdef int rank = np.PyArray_NDIM(data)
    cdef np.npy_intp* shape = np.PyArray_DIMS(data)
    cdef int size = data.dtype.elsize

    put_data.data_type = np.PyArray_TYPE(data)
    put_data.rank = rank
    put_data.count = np.PyArray_SIZE(data)
    put_data.shape = <int *> malloc(rank * size)
    cdef int i = 0
    while i < rank:
        put_data.shape[i] = shape[i]
        i += 1
    put_data.data = np.PyArray_BYTES(data)

    cdef int handle = uda.idamPutAPI(instruction, &put_data)
    return handle


cdef put_string(const char* instruction, const char* data):
    cdef uda.PUTDATA_BLOCK put_data
    uda.initIdamPutDataBlock(&put_data)

    cdef int len = strlen(data)

    put_data.data_type = 17 # UDA_TYPE_STRING
    put_data.rank = 1
    put_data.count = len
    put_data.shape = <int *> malloc(len * sizeof(char))
    put_data.shape[0] = len

    cdef int handle = uda.idamPutAPI(instruction, &put_data)
    return handle


def put_data(instruction, data):
    if isinstance(data, np.ndarray):
        return put_ndarray(instruction, data)
    else:
        return put_string(instruction, data)

