#!/bin/bash

cmake -Bbuild -H. \
    -DTARGET_TYPE=MAST \
    -DCMAKE_INSTALL_PREFIX=/home/uda \
    -DNO_MODULES=ON \
    -DHDF5_ROOT=/usr/local/hdf5 \
    -DSWIG_EXECUTABLE=/home/uda/swig/bin/swig
