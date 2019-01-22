# Set up environment for compilation
. /usr/share/Modules/init/sh
module use /work/imas/etc/modulefiles

module purge

module load GCC/6.4.0-2.28
module load swig/3.0.5
module load hdf5/1.8.15
module load intel/12.0.2

make -C build
