#!/bin/bash

module purge
module load idl/08.3
module load mdsplus/6.1
module load java/1.8
module load udunits/2.2.24
module load ida

export HDF5_USE_SHLIB=yes
export BOOST_ROOT=/usr/local/depot/boost-1.60

cmake3 -Bbuild_freia -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=MAST \
    -DCMAKE_INSTALL_PREFIX=${HOME}/freia \
    -DSWIG_EXECUTABLE=/home/jholloc/freia/bin/swig \
    -DUDUNITS_ROOT=$UDUNITS_DIR \
    $*
