#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module load itmenv

export CC=gcc
export CXX=g++
export BOOST_ROOT=$HOME/boost_1_62_0

cmake -Bbuild-gcc-6 -H. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=/gw/swimas/extra/uda/2.3.1/gcc/6.1.0 \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_PLUGINS=help\;uda $*
