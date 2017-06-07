from __future__ import absolute_import
from logging import DEBUG, WARNING, INFO, ERROR

# noinspection PyUnresolvedReferences
from . import uda_swig
# noinspection PyUnresolvedReferences
from .uda_swig import UDAException
from ._client import Client
from ._signal import Signal
from ._dim import Dim

# import the enum values PROP_* from the uda_swig library into a Properties class
Properties = type('Properties', (), dict((p, getattr(uda_swig, p))
                                         for p in dir(uda_swig) if p.startswith('PROP_')))

__all__ = (UDAException, Client, Signal, Dim, Properties, DEBUG, WARNING, INFO, ERROR)
