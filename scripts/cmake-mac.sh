#!/bin/bash

cmake -GNinja -H. -Bbuild \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=. \
    -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl@1.1/1.1.1m \
    -DCMAKE_BUILD_TYPE=Debug "$@"
