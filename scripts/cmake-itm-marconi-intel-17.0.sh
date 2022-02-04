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

BUILD_DIR="${BUILD_DIR:-build-intel-17}"

cmake -B$BUILD_DIR -H. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=$UDA_INSTALL \
    -DMODULE_NAME=$MODULE_VERSION \
    -DMODULE_PATH=$MODULE_PATH \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_PLUGINS=help\;uda $*
