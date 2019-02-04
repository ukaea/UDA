# Set up environment for compilation
. /usr/share/Modules/init/sh
module use /work/imas/etc/modulefiles

module purge

module load CMake/3.12.1-GCCcore-6.4.0

module load PostgreSQL/10.3-intel-2018a-Python-3.6.4
module load libMemcached/1.0.18-GCCcore-6.4.0
module load SWIG/3.0.12-intel-2018a-Python-3.6.4
module load Boost/1.66.0-intel-2018a
module load netCDF/4.6.0-intel-2018a
module load HDF5/1.10.1-intel-2018a
module load MDSplus-Java/7.46.1-intel-2018a-Java-1.8.0_162

export HDF5_ROOT=$H5DIR

CC=gcc CXX=g++ cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
-DBOOST_ROOT=/work/imas/opt/boost/1.58 \
-DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON

#-DCPACK_GENERATOR=RPM
