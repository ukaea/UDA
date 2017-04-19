#!/bin/bash

eval `tclsh /work/imas/opt/modules-tcl/modulecmd.tcl $(basename $SHELL) autoinit`

module purge
module use /work/imas/etc/modules/all
module load Python/3.4.5-foss-2016a
module load libmemcached/1.0.18
module load cmake/3.0.2
module load postgresql/9.4.4
module load libxml2/2.9.2
module load boost/1.58
module load swig/3.0.5
module load openssl/1.0.2g
module load HDF5/1.10.0-patch1-foss-2016a
module load git/2.6.2

export HDF5_ROOT=$H5DIR

CC=gcc CXX=g++ cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
  -DPYTHON_INCLUDE_DIR=${EBROOTPYTHON}/include/python3.4m/ \
  -DPYTHON_LIBRARY=${EBROOTPYTHON}/lib/libpython3.4m.so \
  -DBOOST_ROOT=/work/imas/opt/boost/1.58 \
  -DCMAKE_INSTALL_PREFIX=. $*
