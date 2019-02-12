#!/bin/bash

cmake -GNinja -H. -Bbuild -DTARGET_TYPE=MAST \
    -DNO_MODULES=ON \
    -DCMAKE_INSTALL_PREFIX=$HOME/Projects/uda-develop \
    -DCMAKE_BUILD_TYPE=Debug $*
