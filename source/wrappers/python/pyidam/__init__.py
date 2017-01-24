from logging import DEBUG, WARNING, INFO, ERROR

from . import cidam
from .cidam import IdamException
from ._client import Client
from ._signal import Signal
from ._dim import Dim
from ._structured import StructuredData
from ._json import SignalEncoder, SignalDecoder

# import the enum values PROP_* from the cidam library into a Properties class
Properties = type('Properties', (), dict((p, getattr(cidam, p))
                                         for p in dir(cidam) if p.startswith('PROP_')))

__all__ = (IdamException, Client, Signal, Dim, Properties, DEBUG, WARNING, INFO, ERROR)
