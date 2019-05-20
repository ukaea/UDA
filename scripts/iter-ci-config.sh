#!/bin/bash
# Bamboo Build script
# Stage 1 : Configure stage

# Set up environment for compilation
. scripts/iter-ci-setup-env.sh || exit 1

CC=icc CXX=icpc cmake -Bbuild -H. \
    -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DBOOST_ROOT=${EBROOTBOOST} \
    -DNO_MODULES=ON \
    -DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON
