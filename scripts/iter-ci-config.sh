# Set up environment for compilation
. /usr/share/Modules/init/sh
module use /work/imas/etc/modulefiles

module purge

module load GCC/6.4.0-2.28
module load libmemcached/1.0.18
module load cmake/3.0.2
module load postgresql/9.4.4
module load libxml2/2.9.2
module load Boost/1.66.0-foss-2018a
module load swig/3.0.5
module load openssl/1.0.2g
module load hdf5/1.8.15
module load git/2.6.2
module load intel/12.0.2

export HDF5_ROOT=$H5DIR

CC=gcc CXX=g++ cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
    -DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON \
    $*

#-DCPACK_GENERATOR=RPM
