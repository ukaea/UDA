#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module unload gnu
module unload itm-gcc
module load cmake/3.5.2
module load itm-boost/1.78.0/gcc/4.8

export CC=gcc
export CXX=g++

cmake -Bbuild-gcc-4 -H. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=$BOOST_INSTALL \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_PLUGINS=help\;uda $*
