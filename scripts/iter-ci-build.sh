#!/bin/bash
# Bamboo Build script
# Stage 2 : Build stage

# Set up environment for compilation
source ./scripts/iter-ci-setup-env.sh || exit 1

if [ "$OS" == "Windows_NT" ]
then
	# Make portable XDR
	cd extlib/portablexdr-4.9.1
	autoconf
	./configure
	make
	
	# Deploy portable XDR libraries
	cd ..
	./install.sh
	
	# Return to UDA level
	cd ..
fi

# Make UDA
make -C build
