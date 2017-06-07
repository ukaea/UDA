from __future__ import absolute_import
import numpy as np

# noinspection PyUnresolvedReferences
from .uda_swig import UDAException


def cdata_scalar_to_value(scalar):
    """
    Convert an UDA C++ Scalar object to an equivalent python type.

    :param scalar: an UDA C++ scalar as wrapped by the low level uda_swig library
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


def cdata_vector_to_value(vector):
    """
    Convert an UDA C++ Vector object to an equivalent python type.

    :param vector: an UDA C++ vector as wrapped by the low level uda_swig library
    :return: a list of numbers or strings
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
        # converting SWIG vector<char*> to list of strings
        return [vec[i] for i in range(len(vec))]
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
