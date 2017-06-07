"""
Class to return signals that should be read in for top-level groups
(ie. groups that come from more than one file).
Also returns the appropriate manipulation classes for the signals requested.
"""
from __future__ import absolute_import

from ._geomPickup import GeomPickup
from ._geomFluxloops import GeomFluxloops


class GeometryFiles:
    def __init__(self):
        """
        Init function
        :return:
        """
        self._signal_manip_map = {}
        self._build_map()

    # --------------------------
    def _build_map(self):
        """
        Defines maps
        :return:
        """
        # Map from top-level groups in each file to the appropriate manipulator
        self._signal_manip_map = {'/magnetics/pickup': GeomPickup(),
                                  '/magnetics/mirnov': GeomPickup(),
                                  '/magnetics/fluxloops': GeomFluxloops()}

    # --------------------------
    def get_signals(self, signals):
        """
        From overall signal that was asked for, retrieve
        appropriate manipulation classes
        :param signals: Signal user asked for
        :return:
        """
        # Find manipulators for those files
        keys = self._signal_manip_map.keys()
        manip = [None] * len(signals)
        for index, sig in enumerate(signals):

            signal = sig.rstrip('/')
            if signal[0] != '/':
                signal = '/' + signal

            if signal in keys:
                manip[index] = self._signal_manip_map[signal]

        return manip
