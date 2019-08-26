#!/bin/bash
# Bamboo Build script

if [ "$OS" == "Windows_NT" ]
then
	echo "MSYS2/MinGW environnement under Windows"
	
	# Convert Windows style path to Linux
	export JAVA_HOME=`cygpath.exe -u "$JAVA_HOME"`
	export JAVA_INCLUDE_PATH=$JAVA_HOME/include
	export JAVA_INCLUDE_PATH2=$JAVA_HOME/include/win32
	export JAVA_AWT_INCLUDE_PATH=$JAVA_INCLUDE_PATH
	export JNI_INCLUDE_DIR=$JAVA_INCLUDE_PATH
	export JNI_MD_INCLUDE_DIR=$JAVA_INCLUDE_PATH/win32
	export JNI_LIB_DIR=$AVA_HOME/lib
	export JAVA_AWT_LIBRARY=$NI_LIB_DIR/jawt.lib
	export JAVA_JVM_LIBRARY=$JNI_LIB_DIR/jvm.lib
	export PATH=$AVA_HOME/bin:$PATH
	
	export HDF5_ROOT=`cygpath.exe -u "$HDF5_ROOT"`
	export PostgresSQL_ROOT=`cygpath.exe -u "$PostgresSQL_ROOT"`
	export NETCDF_DIR=`cygpath.exe -u "$NETCDF_DIR"`
	
	
	echo "JAVA_HOME: $JAVA_HOME"
	echo "HDF5_ROOT: $HDF5_ROOT"
	echo "PostgresSQL_ROOT: $PostgresSQL_ROOT"
	echo "NETCDF_DIR: $NETCDF_DIR"
else
	echo "Linux environnement"
	# Stage 0 : load modules

	# Set up environment for compilation
	. /usr/share/Modules/init/sh
	module use /work/imas/etc/modulefiles
	module use /work/imas/etc/modules/all
	module purge

	# FOSS environment based upon GCC v6.4.0

	module load CMake/3.12.1-GCCcore-6.4.0

	module load PostgreSQL/10.3-foss-2018a-Python-2.7.14
	module load libMemcached/1.0.18-GCCcore-6.4.0
	module load SWIG/3.0.12-foss-2018a-Python-2.7.14
	module load Boost/1.66.0-foss-2018a
	module load netCDF/4.6.0-foss-2018a
	module load HDF5/1.10.1-foss-2018a
	module load MDSplus-Java/7.49.1-GCCcore-6.4.0-Java-1.8.0_162	
fi
