#!/bin/bash
module purge
module load cineca
module load mdsplus/7.46-1 
module load cmake/3.5.2
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DCMAKE_INSTALL_PREFIX=. \
    -DBUILD_PLUGINS=help $* 

