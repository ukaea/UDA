from __future__ import (division, print_function, absolute_import)

import abc

from future import standard_library
from future.utils import with_metaclass
standard_library.install_aliases()


class Data(with_metaclass(abc.ABCMeta, object)):
    """
    The base class of data that can be returned by the pyuda Client.
    """

    @abc.abstractmethod
    def plot(self):
        pass

    @abc.abstractmethod
    def widget(self):
        pass

    def jsonify(self, indent=None):
        raise NotImplementedError("jsonify has not been implement for this data class")
