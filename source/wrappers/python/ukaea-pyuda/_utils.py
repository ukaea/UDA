from __future__ import (division, print_function, absolute_import)

import numpy as np

import cpyuda


UDAException = cpyuda.UDAException


def cdata_scalar_to_value(scalar):
    """
    Convert a UDA C++ Scalar object to an equivalent python type.

    :param scalar: a UDA C++ scalar as wrapped by the low level c_uda library
    :return: a number or string
    """
    if scalar.type() == 'float32':
        return scalar.fdata()
    elif scalar.type() == 'float64':
        return scalar.ddata()
    elif scalar.type() == 'int8':
        return scalar.cdata()
    elif scalar.type() == 'uint8':
        return scalar.ucdata()
    elif scalar.type() == 'int16':
        return scalar.sdata()
    elif scalar.type() == 'uint16':
        return scalar.usdata()
    elif scalar.type() == 'int32':
        return scalar.idata()
    elif scalar.type() == 'uint32':
        return scalar.uidata()
    elif scalar.type() == 'int64':
        return scalar.ldata()
    elif scalar.type() == 'uint64':
        return scalar.uldata()
    elif scalar.type() == 'string':
        return scalar.string()
    else:
        raise UDAException("Unknown data type " + scalar.type())


def cdata_array_to_value(array):
    """
    Convert an UDA C++ Array object to an equivalent python type.

    :param array: an UDA C++ array as wrapped by the low level c_uda library
    :return: a numpy array
    """
    if array.type() == 'float32':
        return np.array(array.fdata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'float64':
        return np.array(array.ddata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'int8':
        return np.array(array.cdata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'uint8':
        return np.array(array.ucdata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'int16':
        return np.array(array.sdata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'uint16':
        return np.array(array.usdata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'int32':
        return np.array(array.idata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'uint32':
        return np.array(array.uidata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'int64':
        return np.array(array.ldata(), dtype=array.type()).reshape(array.shape())
    elif array.type() == 'uint64':
        return np.array(array.uldata(), dtype=array.type()).reshape(array.shape())
    else:
        raise UDAException("Unknown data type " + array.type())


def cdata_vector_to_value(vector):
    """
    Convert an UDA C++ Vector object to an equivalent python type.

    :param vector: an UDA C++ vector as wrapped by the low level c_uda library
    :return: a numpy array
    """
    if vector.type() == 'float32':
        return np.array(vector.fdata(), dtype=vector.type())
    elif vector.type() == 'float64':
        return np.array(vector.ddata(), dtype=vector.type())
    elif vector.type() == 'int8':
        return np.array(vector.cdata(), dtype=vector.type())
    elif vector.type() == 'uint8':
        return np.array(vector.ucdata(), dtype=vector.type())
    elif vector.type() == 'int16':
        return np.array(vector.sdata(), dtype=vector.type())
    elif vector.type() == 'uint16':
        return np.array(vector.usdata(), dtype=vector.type())
    elif vector.type() == 'int32':
        return np.array(vector.idata(), dtype=vector.type())
    elif vector.type() == 'uint32':
        return np.array(vector.uidata(), dtype=vector.type())
    elif vector.type() == 'int64':
        return np.array(vector.ldata(), dtype=vector.type())
    elif vector.type() == 'uint64':
        return np.array(vector.uldata(), dtype=vector.type())
    elif vector.type() == 'string':
        vec = vector.string()
        return [vec[i] for i in range(len(vec))]  # converting SWIG vector<char*> to list of strings
    else:
        raise UDAException("Unknown data type " + vector.type())


def cdata_to_numpy_array(cdata):
    if cdata.type() == 'float32':
        return np.array(cdata.fdata(), dtype=cdata.type())
    elif cdata.type() == 'float64':
        return np.array(cdata.ddata(), dtype=cdata.type())
    elif cdata.type() == 'int8':
        return np.array(cdata.cdata(), dtype=cdata.type())
    elif cdata.type() == 'uint8':
        return np.array(cdata.ucdata(), dtype=cdata.type())
    elif cdata.type() == 'int16':
        return np.array(cdata.sdata(), dtype=cdata.type())
    elif cdata.type() == 'uint16':
        return np.array(cdata.usdata(), dtype=cdata.type())
    elif cdata.type() == 'int32':
        return np.array(cdata.idata(), dtype=cdata.type())
    elif cdata.type() == 'uint32':
        return np.array(cdata.uidata(), dtype=cdata.type())
    elif cdata.type() == 'int64':
        return np.array(cdata.ldata(), dtype=cdata.type())
    elif cdata.type() == 'uint64':
        return np.array(cdata.uldata(), dtype=cdata.type())
    elif cdata.type() == 'string':
        return (''.join(cdata.cdata()))[:-1]
    else:
        raise UDAException("Unknown data type " + cdata.type())
