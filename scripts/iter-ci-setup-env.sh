#!/bin/bash
# Bamboo Build script
# Stage 0 : load modules

# Set up environment for compilation
. /usr/share/Modules/init/sh
module use /work/imas/etc/modulefiles
module use /work/imas/etc/modules/all
module purge

# FOSS environment based upon GCC v6.4.0

module load CMake/3.12.1-GCCcore-6.4.0

module load libMemcached/1.0.18-GCCcore-6.4.0
module load SWIG/3.0.12-foss-2018a-Python-2.7.14
module load Boost/1.66.0-foss-2018a
module load intel/17.0.4
