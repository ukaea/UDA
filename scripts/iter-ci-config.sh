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
		-DOPENSSL_INCLUDE_DIR=/usr/include -DOPENSSL_SSL_LIBRARY=/usr/lib/libssl.a -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/libcrypto.a \
		-DLIBXML2_INCLUDE_DIR=/mingw64/include -DLIBXML2_LIBRARY=/mingw64/lib/libxml2.a \
		-DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER \
		-DBUILD_SHARED_LIBS=ON -DNO_MODULES=ON \
		-DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON
else
	echo "Linux environnement"
	
	cmake -Bbuild -H. \
		-DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=OTHER -DBOOST_ROOT=${EBROOTBOOST} \
		-DHDF5_ROOT=${EBROOTHDF5} -DPostgreSQL_ROOT=${EBROOTPOSTGRESQL} -DNETCDF_DIR=${EBROOTNETCDF} \
		-DCMAKE_INSTALL_PREFIX=. -DITER_CI=ON
fi
