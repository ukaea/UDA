#!/bin/bash

module ()
{
    eval `/usr/bin/modulecmd bash $*`
}

module purge
module load cineca
module load itmenv
#module load boost/1.61.0--intelmpi--2017--binary

export CC=gcc
export CXX=g++
export BOOST_ROOT=$HOME/boost_1_62_0

cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
    -DNO_MODULES=ON \
    -DSWIG_EXECUTABLE:FILEPATH=/afs/eufus.eu/user/g/g2jhollo/bin/swig \
    -DCMAKE_INSTALL_PREFIX=/gw/swimas/extra/uda/develop \
    -DBUILD_PLUGINS=help\;uda $*
