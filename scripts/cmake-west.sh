#!/bin/bash

source /usr/share/Modules/init/bash
module purge
module use /Applications/Modules/soft
module use /Applications/Modules/compilers
module load gcc/6.4.0
module load mdsplus/7.46-4 
module load cmake/3.7.2 
export BOOST_ROOT=/Applications/boost_1_69_0
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DCMAKE_INSTALL_PREFIX=. \
    -DBUILD_PLUGINS=help $* 

