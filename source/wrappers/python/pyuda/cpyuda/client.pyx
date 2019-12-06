#cython: language_level=3

cimport uda


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
    return Result(handle)


def put_data(signal, source, data):
    pass
