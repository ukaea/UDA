from __future__ import (division, print_function, absolute_import)

from builtins import object
from future import standard_library
standard_library.install_aliases()

import math
import numpy as np


class GeomEfitLimiter(object):
    """
    Manipulation class for efit limiter

    Plotting:

    Plot the limiting surface (2D only atm)
    """

    def __init__(self):
        pass

    def do_manip(self, data, **kwargs):
        pass

    def _plot_elements(self, data, ax_2d, color=None):
        """
        Recursively loop over tree, and retrieve EFIT limiting surface.
        :param data: data tree (instance of StructuredWritable, with EFIT limiter tree structure)
        :return:
        """
        if hasattr(data, "R"):
            R = data.R
            Z = data.Z

            ax_2d.plot(R, Z, color=color, linewidth=1)
        else:
            for child in data.children:
                self._plot_elements(child, ax_2d, color=color)

    def plot(self, data, ax_2d=None, ax_3d=None, show=True, color=None):
        # Create axes if necessary
        import matplotlib.pyplot as plt
        
        if ax_2d is None:
            fig = plt.figure()
            if ax_2d is None:
                ax_2d = fig.add_subplot(111)

        if color is None:
            color = "black"

        # Plot
        if ax_2d is not None:
            self._plot_elements(data, ax_2d, color=color)

            ax_2d.set_xlabel('R [m]')
            ax_2d.set_ylabel('Z [m]')
            ax_2d.set_aspect('equal', 'datalim')

        if ax_3d is not None:
            pass

        if show:
            plt.show()

