"""
Utilities for geometry manipulations, that are useful across geometry manipulation classes.
"""

import math
import numpy as np

def unit_vector_to_poloidal_angle(R, Z):
    """
    Take a unit vector in cylindrical co-ordinates
    and calculate the "poloidal angle".
    :param R: R element
    :param Z: Z element
    :return: poloidal angle
    """
    try:
        theta = math.acos(R/math.sqrt(Z*Z+R*R))*180.0/math.pi
    except ZeroDivisionError:
        theta = 0.0

    if Z > 0.0:
        theta = 360.0 - theta

    return theta

def vector_to_bR_bZ_bPhi(orientation):
    """
    Take a unit vector in cylindrical co-ordinates
    and calculate the fraction of the vector
    that is in the R and Z directions.
    :param orientation : orientation element, containing unit vector in direction of measurement
    :return: (bRFraction, bZFraction, bPhiFraction)
    """
    # First, calculate how much is toroidal/poloidal
    norm = (math.fabs(orientation["unit_vector"].phi)
            + math.fabs(orientation["unit_vector"].r)
            + math.fabs(orientation["unit_vector"].z))
    bPhiFraction = orientation["unit_vector"].phi/norm
    bZFraction = orientation["unit_vector"].z/norm
    bRFraction = orientation["unit_vector"].r/norm

    return bRFraction, bZFraction, bPhiFraction

def length_poloidal_projection(length, orientation):
    """
    Using the unit vector describing the orientation
    of the object, calculate the projection of the
    length of the object in the poloidal plane.
    :param length: length of object
    :param orientation: orientation of the object
    :return: projected length
    """
    angle_to_poloidal_plane = math.asin(orientation["unit_vector"].phi)
    fraction_poloidal_plane = math.cos(angle_to_poloidal_plane)

    new_length = length*fraction_poloidal_plane

    if np.isclose(new_length, 0.0):
        new_length = 0.0

    return new_length

def cylindrical_cartesian(coordinate):
    """
    Take cylindrical coordinate and translate to cartesian coordinate
    :param coordinate: coordinate node.
    :return: [x,y,z]
    """
    x = coordinate.r * math.cos(coordinate.phi*math.pi/180.0)
    y = coordinate.r * math.sin(coordinate.phi*math.pi/180.0)
    z = coordinate.z

    return [x,y,z]

