#!/bin/bash

module purge
module load idl/08.4
module load java/1.8
module load hdf5-C/1.8.13
module load gcc/7.3.0
module load cmake/3.21.0
module load capnproto/0.10.4
module load fmt/10.0.0
module load spdlog/1.11.0

export HDF5_USE_SHLIB=yes
export BOOST_ROOT=/usr/local/depot/boost-1-77-0-gcc7.3.0 

CC=gcc CXX=g++ cmake -Bbuild_freia -H. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/projects/UDA/uda-install-develop \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_CAPNP=ON \
    -DUDA_CLI_BOOST_STATIC=ON \
    -DUDA_HOST=uda2.mast.l \
    -DUDA_PORT=56565 \
    $*
