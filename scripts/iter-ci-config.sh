#!/bin/bash
# Bamboo Build script
# Stage 1 : Configure stage

# Set up environment for compilation
. scripts/iter-ci-setup-env.sh || exit 1

cmake -Bbuild -H. \
    -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DBOOST_ROOT=${EBROOTBOOST} \
    -DHDF5_ROOT=${EBROOTHDF5} -DPostgreSQL_ROOT=${EBROOTPOSTGRESQL} -DNETCDF_DIR=${EBROOTNETCDF}       \
    -DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON
