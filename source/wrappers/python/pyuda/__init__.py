from __future__ import (division, print_function, absolute_import)

from logging import DEBUG, WARNING, INFO, ERROR

import cpyuda

from ._client import Client, ListType
from ._signal import Signal
from ._dim import Dim
from ._structured import StructuredData
from ._json import SignalEncoder, SignalDecoder


UDAException = cpyuda.UDAException
ProtocolException = cpyuda.ProtocolException
ServerException = cpyuda.ServerException
InvalidUseException = cpyuda.InvalidUseException


# import the enum values PROP_* from the c_uda library into a Properties class
Properties = type('Properties', (), dict(
    (p, getattr(cpyuda, p)) for p in dir(cpyuda) if p.startswith('PROP_')
))

__all__ = (UDAException, ProtocolException, ServerException, InvalidUseException,
           Client, ListType, Signal, Dim, Properties, DEBUG, WARNING, INFO, ERROR)
