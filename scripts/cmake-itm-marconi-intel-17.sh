#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module load itm-intel/17.0
module load cmake/3.5.2

export CC=icc
export CXX=icpc

cmake -Bbuild-intel-17 -H. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=$UDA_INSTALL \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_PLUGINS=help\;uda $*
