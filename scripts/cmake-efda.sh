#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module load mdsplus
module load cmake/3.5.2

export CC=gcc
export CXX=g++
export BOOST_ROOT=$HOME/itmwork/boost_1_62_0

cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
    -DNO_MODULES=ON \
    -DLIBSSH_ROOT=/afs/eufus.eu/user/g/g2jhollo \
    -DCMAKE_INSTALL_PREFIX=$HOME/uda \
    -DBUILD_PLUGINS=imas\;help\;exp2imas\;west_tunnel $*
