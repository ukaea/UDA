#!/bin/bash

module purge
module load idl/08.3

export CPLUS_INCLUDE_PATH=/usr/include/libxml2:$CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=/usr/include/libxml2:$C_INCLUDE_PATH

CC=gcc CXX=g++ cmake3 -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/root/sharepkg/U/uda/develop \
  -DCLIENT_ONLY=TRUE \
  -DBUILD_SHARED_LIBS=ON \
  
