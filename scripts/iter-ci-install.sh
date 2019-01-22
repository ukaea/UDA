# Set up environment for compilation
. /usr/share/Modules/init/sh
module use /work/imas/etc/modulefiles

module purge

module load GCC/6.4.0-2.28

make -C build install
