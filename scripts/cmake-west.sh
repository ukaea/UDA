#!/bin/bash

source /usr/share/Modules/init/bash
module purge
module use /Applications/Modules/soft
module use /Applications/Modules/compilers
module load gcc/6.4.0
module load mdsplus/7.7-4 
module load cmake/3.7.2 
export BOOST_ROOT=/Applications/boost_1_66_0
cmake -Bbuild -H. -DTARGET_TYPE=OTHER -DCMAKE_INSTALL_PREFIX=. \
    -DBUILD_PLUGINS=help $* 
cmake -Bbuild -H. -DTARGET_TYPE=OTHER -DCMAKE_INSTALL_PREFIX=. \
    -DLIBTS_ROOT=/Home/devarc/PortageMatlab7/tslib_client2013 \
    -DBUILD_PLUGINS=imas\;help\;west $*
#cmake -Bbuild -H. -DTARGET_TYPE=OTHER -DCMAKE_INSTALL_PREFIX=. -DLIBTS_ROOT=/Home/devarc/PortageMatlab7/tslib_client2013
