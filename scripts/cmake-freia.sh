#!/bin/bash

module purge
module load idl/08.3
module load mdsplus/6.1
module load java/1.8
module load udunits/2.2.24
module load python/3.3.5
module load ida

export HDF5_USE_SHLIB=yes
export BOOST_ROOT=/usr/local/depot/boost-1.60

VERSION=`git describe --always --dirty --tags`

cmake3 -Bbuild_freia -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=MAST \
    -DCMAKE_INSTALL_PREFIX=${HOME}/freia \
    -DSWIG_EXECUTABLE=/home/jholloc/freia/bin/swig \
    -DUDUNITS_ROOT=$UDUNITS_DIR \
    -DPYTHON_INCLUDE_DIR=/usr/local/depot/Python-3.3.5/include/python3.3m/ -DPYTHON_LIBRARY=/usr/local/depot/Python-3.3.5/lib/libpython3.3m.so \
    $*

#    -DCLIENT_ONLY=TRUE \
#    -DCMAKE_INSTALL_PREFIX=/common/IMAS/extra/uda/$VERSION \
#-DIMAS_ROOT=$HOME/Projects/CPT/installer_hdf5/src/master/3/UAL \
    #-DMDSPLUS_DIR=/usr/local/depot/mdsplus-5.0 \