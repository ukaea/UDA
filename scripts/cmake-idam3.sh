#!/bin/bash

CC=/var/home/idam/bin/gcc CXX=/var/home/idam/bin/g++ cmake \
    -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug \
    -DTARGET_TYPE=MAST \
    -DCMAKE_INSTALL_PREFIX=. \
    -DBUILD_SHARED_LIBS=ON \
    -DNO_MODULES=ON
