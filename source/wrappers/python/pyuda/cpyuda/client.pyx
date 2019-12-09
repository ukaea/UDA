#cython: language_level=3

cimport uda
from libc cimport string


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


def put_data(signal, source, data):
    pass
