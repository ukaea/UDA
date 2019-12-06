#!/bin/bash

mkdir -p build
rm -rf build/*

cmake -H. -Bbuild \
	-G"Unix Makefiles" \
	-DTARGET_TYPE=OTHER -DNO_MODULES=ON \
	-DCMAKE_INSTALL_PREFIX=$PWD/install \
	-DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=Debug

