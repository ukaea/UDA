#!/bin/bash
# Bamboo Build script
# Stage 1 : Configure stage

# Set up environment for compilation (source to keep exportation)
source ./scripts/iter-ci-setup-env.sh || exit 1

if [ "$OS" == "Windows_NT" ]
then
	echo "MSYS2/MinGW environnement under Windows"
	
	cmake -Bbuild -H. -G"Unix Makefiles" \
		-DHDF5_ROOT=${HDF5_ROOT} -DPostgreSQL_ROOT=${PostgresSQL_ROOT} -DNETCDF_DIR=${NETCDF_DIR} \
		-DMDSPLUS_DIR=${MDSPLUS_DIR} \
		-DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
		-DNO_MODULES=ON -DBUILD_SHARED_LIBS=ON -DNO_JAVA_WRAPPER=ON \
		-DCMAKE_INSTALL_PREFIX=${PWD}/install -DITER_CI=ON
else
	echo "Linux environnement"
	
	cmake -Bbuild -H. \
		-DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DBOOST_ROOT=${EBROOTBOOST} \
		-DHDF5_ROOT=${EBROOTHDF5} -DPostgreSQL_ROOT=${EBROOTPOSTGRESQL} -DNETCDF_DIR=${EBROOTNETCDF} \
		-DBUILD_SHARED_LIBS=ON \
		-DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON
fi
