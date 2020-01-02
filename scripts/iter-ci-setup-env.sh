#!/bin/bash
# Bamboo Build script

if [ "$OS" == "Windows_NT" ]
then
	echo "MSYS2/MinGW environnement under Windows"
	
	if [ -z "$JAVA_HOME" ]
	then
		echo "ERROR: Variable JAVA_HOME is empty!"
		exit 1
	fi
	if [ -z "$HDF5_ROOT" ]
	then
		echo "ERROR: Variable HDF5_ROOT is empty!"
		exit 1
	fi
	if [ -z "$MDSPLUS_DIR" ]
	then
		echo "ERROR: Variable MDSPLUS_DIR is empty!"
		exit 1
	fi
	if [ -z "$PostgresSQL_ROOT" ]
	then
		echo "ERROR: Variable PostgresSQL_ROOT is empty!"
		exit 1
	fi
	if [ -z "$NETCDF_DIR" ]
	then
		echo "ERROR: Variable NETCDF_DIR is empty!"
		exit 1
	fi
	
	# Convert Windows style path to Linux
	export JAVA_HOME=`cygpath.exe -u "$JAVA_HOME"`
	export HDF5_ROOT=`cygpath.exe -u "$HDF5_ROOT"`
	export MDSPLUS_DIR=`cygpath.exe -u "$MDSPLUS_DIR"`
	export PostgresSQL_ROOT=`cygpath.exe -u "$PostgresSQL_ROOT"`
	export NETCDF_DIR=`cygpath.exe -u "$NETCDF_DIR"`
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
