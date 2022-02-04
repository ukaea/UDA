#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module unload gnu
module load gnu/7.3.0
module load itm-openmpi/4.0.4--gnu--7.3.0
module load itm-boost/1.61.0--gnu--7.3.0
module load cmake/3.5.2

export CC=gcc
export CXX=g++

BUILD_DIR="${BUILD_DIR:-build-gcc-7.3.0}"

cmake -B$BUILD_DIR -H. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=$UDA_INSTALL \
    -DMODULE_NAME=$MODULE_VERSION \
    -DMODULE_PATH=$MODULE_PATH \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_PLUGINS=help\;uda $*
