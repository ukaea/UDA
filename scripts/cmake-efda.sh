#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module load imasenv
module switch mdsplus/alpha

export CC=gcc
export CXX=g++
export BOOST_ROOT=$HOME/itmwork/boost_1_62_0

cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
    -DNO_MODULES=ON \
    -DCMAKE_INSTALL_PREFIX=$HOME/uda -DBUILD_PLUGINS=help\;imas $*
