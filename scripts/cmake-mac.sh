#!/bin/bash

cmake -GNinja -H. -Bbuild \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=. \
    -DCMAKE_BUILD_TYPE=Debug "$@"
