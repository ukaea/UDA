"""
Class to return signals that should be read in for top-level groups
(ie. groups that come from more than one file).
Also returns the appropriate manipulation classes for the signals requested.
"""

import numpy as np
from ._geomPickup import GeomPickup


class GeometryFiles:
    def __init__(self):
        """
        Init function
        :return:
        """
        self._signal_groups_map = {}
        self._signal_manip_map = {}
        self._build_map()

    # --------------------------
    def _build_map(self):
        """
        Defines maps
        :return:
        """
        # Map from top-level groups to level of files
        self._signal_groups_map = {'/magnetics': ['/magnetics/pickup']}  # , '/magnetics/fluxloops']}

        # Map from top-level groups in each file to the appropriate manipulator
        self._signal_manip_map = { '/magnetics/pickup': GeomPickup() }#,
                                   # '/magnetics/fluxloops': None }

    # --------------------------
    def get_signals(self, signal):
        """
        From overall signal that was asked for, retrieve
        file-level signals that should be read in.
        Also, return appropriate manipulation classes
        :param signal: Signal user asked for
        :return:
        """
        signal = signal.rstrip('/')
        if signal[0] != '/':
            signal = '/' + signal

        # First, check if signal needs more than one file
        # (needed since at the moment can't combine files in idam server code)
        try:
            all_signals = self._signal_groups_map[signal]
        except KeyError:
            all_signals = [signal]

        # Find manipulators for those files
        keys = self._signal_manip_map.keys()
        manip = [None]*len(all_signals)
        for index, sig in enumerate(all_signals):

            if sig in keys:
                manip[index] = self._signal_manip_map[sig]
            else:
                for key in keys:
                    if sig.startswith(key):
                        manip[index] = self._signal_manip_map[key]

        return all_signals, manip