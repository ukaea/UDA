#!/bin/bash

export CC=gcc
export CXX=g++
export BOOST_ROOT=$HOME/itmwork/boost_1_62_0

cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DSWIG_EXECUTABLE=/afs/eufus.eu/user/g/g2jhollo/bin/swig \
-DPYTHON_INCLUDE_DIR=/cineca/prod/opt/compilers/python/3.5.2/none/include/python3.5m \
-DPYTHON_LIBRARY=/cineca/prod/opt/compilers/python/3.5.2/none/lib/libpython3.5m.so \
-DCMAKE_INSTALL_PREFIX=$HOME/uda $*
