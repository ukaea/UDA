#!/bin/bash

module purge
module load idl/08.3
module load mdsplus/6.1
module load java/1.8
module load ida
module load hdf5-C/1.8.13
module load netcdf-C/4.3.2

export HDF5_USE_SHLIB=yes
export BOOST_ROOT=/usr/local/depot/boost-1.60
export CPLUS_INCLUDE_PATH=/usr/include/libxml2:$CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=/usr/include/libxml2:$C_INCLUDE_PATH

CC=gcc CXX=g++ cmake3 -Bbuild_freia -H. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=/projects/UDA/uda-install-develop \
    -DLIBMEMCACHED_ROOT=/common/projects/UDA/libmemcached \
    -DBUILD_SHARED_LIBS=ON \
    $*
