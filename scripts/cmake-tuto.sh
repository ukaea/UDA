#!/bin/bash

module purge
#module load gcc/6.4.0
module load cineca
module load mdsplus/7.46-1 
module load cmake/3.5.2
#module load boost/1.66.0--intelmpi--2018--binary 
#export BOOST_ROOT=/Applications/boost_1_69_0
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DCMAKE_INSTALL_PREFIX=. \
    -DBUILD_PLUGINS=help $* 

