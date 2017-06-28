"""
Manipulation class for efit elements (ie. rectangles and parallelograms!).

Plotting:

Plot the positions of the elements
"""

import math

import matplotlib.pyplot as plt
import numpy as np

class GeomEfitElements():
    def __init__(self):
        pass

    def do_manip(self, data, **kwargs):
        pass

    def _plot_elements(self, data, ax_2d, color=None):
        """
        Recursively loop over tree, and retrieve element geometries.
        :param data: data tree (instance of StructuredWritable, with EFIT element tree structure)
        :return:
        """

        if color is None:
            color = "red"

        if hasattr(data, "centreR"):
            centreRall = data.centreR
            centreZall = data.centreZ
            dRall = data.dR / 2
            dZall = data.dZ / 2

            # Try to retrieve angles (only parallelogram elements have this!)
            try:
                angle1all = data.shapeAngle1
                angle2all = data.shapeAngle2
            except AttributeError:
                if hasattr(centreRall, "__len__"):
                    angle1all = np.zeros(len(centreRall))
                    angle2all = np.zeros(len(centreRall))
                else:
                    angle1all = 0.0
                    angle2all = 0.0

            try:
                for centreR, centreZ, dR, dZ, a1, a2 in zip(centreRall, centreZall, dRall, dZall, angle1all, angle2all):
                    if a1 == 0.0 and a2 == 0.0:
                        # Rectangle
                        rr = [centreR - dR, centreR - dR, centreR + dR, centreR + dR, centreR - dR]
                        zz = [centreZ - dZ, centreZ + dZ, centreZ + dZ, centreZ - dZ, centreZ - dZ]
                    else:
                        # Parallelogram
                        Lx1 = (math.cos(math.radians(a1)) * dR * 2)
                        Lx2 = (math.sin(math.radians(a2)) * dZ * 2)
                        Lx = Lx1 + Lx2

                        Lz1 = (math.sin(math.radians(a1)) * dR * 2)
                        Lz2 = (math.cos(math.radians(a2)) * dZ * 2)
                        Lz = Lz1 + Lz2

                        A = (centreR - Lx / 2, centreZ - Lz / 2)
                        B = (centreR - Lx / 2 + Lx2, centreZ - Lz / 2 + Lz2)
                        C = (centreR + Lx / 2, centreZ + Lz / 2)
                        D = (centreR - Lx / 2 + Lx1, centreZ - Lz / 2 + Lz1)
                        rr = [centreR - Lx / 2,        # A
                              centreR - Lx / 2 + Lx2,  # B
                              centreR + Lx / 2,        # C
                              centreR - Lx / 2 + Lx1,  # D
                              centreR - Lx / 2]        # A

                        zz = [centreZ - Lz / 2,
                              centreZ - Lz / 2 + Lz2,
                              centreZ + Lz / 2,
                              centreZ - Lz / 2 + Lz1,
                              centreZ - Lz / 2]

                    ax_2d.plot(rr, zz, color=color, linewidth=0.5)
                    ax_2d.plot(centreR, centreZ, marker="+", color=color, linewidth=0, markersize=1)

            except TypeError:
                if angle1all == 0.0 and angle2all == 0.0:
                    rr = [centreRall - dRall, centreRall - dRall, centreRall + dRall,
                          centreRall + dRall, centreRall - dRall]
                    zz = [centreZall - dZall, centreZall + dZall,
                          centreZall + dZall, centreZall - dZall, centreZall - dZall]
                else:
                    Lx1 = (math.cos(math.radians(angle1all)) * dRall * 2)
                    Lx2 = (math.sin(math.radians(angle2all)) * dZall * 2)
                    Lx = Lx1 + Lx2

                    Lz1 = (math.sin(math.radians(angle1all)) * dRall * 2)
                    Lz2 = (math.cos(math.radians(angle2all)) * dZall * 2)
                    Lz = Lz1 + Lz2

                    A = (centreRall - Lx / 2, centreZall - Lz / 2)
                    B = (centreRall - Lx / 2 + Lx2, centreZall - Lz / 2 + Lz2)
                    C = (centreRall + Lx / 2, centreZall + Lz / 2)
                    D = (centreRall - Lx / 2 + Lx1, centreZall - Lz / 2 + Lz1)
                    rr = [centreRall - Lx / 2,  # A
                          centreRall - Lx / 2 + Lx2,  # B
                          centreRall + Lx / 2,  # C
                          centreRall - Lx / 2 + Lx1,  # D
                          centreRall - Lx / 2]  # A

                    zz = [centreZall - Lz / 2,
                          centreZall - Lz / 2 + Lz2,
                          centreZall + Lz / 2,
                          centreZall - Lz / 2 + Lz1,
                          centreZall - Lz / 2]

                ax_2d.plot(rr, zz, color=color, linewidth=0.5)
                ax_2d.plot(centreRall, centreZall, marker="+", color=color, linewidth=0, markersize=1)
        else:
            for child in data.children:
                self._plot_elements(child, ax_2d, color=color)

    def plot(self, data, ax_2d=None, ax_3d=None, show=True, color=None):
        """
        Plot the elements
        :param data: data tree (instance of StructuredWritable, with EFIT element tree structure)
        :param ax_2d: Axis on which to plot elements in R-Z (2D) plane.
                      If None, then an axis will be created.
        """

        # Create axes if necessary
        if ax_2d is None:
            fig = plt.figure()
            if ax_2d is None:
                ax_2d = fig.add_subplot(111)

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


