from logging import DEBUG, WARNING, INFO, ERROR

from . import c_uda
from .c_uda import UDAException
from ._client import Client
from ._signal import Signal
from ._dim import Dim
from ._structured import StructuredData
from ._json import SignalEncoder, SignalDecoder

# import the enum values PROP_* from the c_uda library into a Properties class
Properties = type('Properties', (), dict((p, getattr(c_uda, p))
                                         for p in dir(c_uda) if p.startswith('PROP_')))

__all__ = (UDAException, Client, Signal, Dim, Properties, DEBUG, WARNING, INFO, ERROR)
