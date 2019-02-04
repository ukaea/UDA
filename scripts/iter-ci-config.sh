#!/bin/bash
# Bamboo Build script
# Stage 1 : Configure stage

# Set up environment for compilation
. scripts/iter-ci-setup-env.sh || exit 1

export HDF5_ROOT=$HDF5_DIR


cmake -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DBOOST_ROOT=${EBROOTBOOST} \
      -DPostgreSQL_ROOT=${EBROOTPOSTGRESQL} -DNETCDF_DIR=${EBROOTNETCDF}       \
      -DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON
