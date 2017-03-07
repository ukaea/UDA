#!/bin/bash

cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=MAST -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/home/jholloc/IdamInstall \
  -DCMAKE_C_FLAGS="-gdwarf-3" -DCMAKE_CXX_FLAGS="-gdwarf-3" $* \
  -DPYTHON_INCLUDE_DIR=/usr/include/python3.4m/ -DPYTHON_LIBRARY=/usr/lib64/libpython3.4m.so