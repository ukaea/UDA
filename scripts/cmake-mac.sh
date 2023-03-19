#!/bin/bash

cmake -GNinja -H. -Bbuild \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=. \
    -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@1.1)" \
    -DCMAKE_BUILD_TYPE=Debug "$@"
