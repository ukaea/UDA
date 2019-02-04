#!/bin/bash
# Bamboo Build script
# Stage 3 : Install stage

# Set up environment for compilation
. scripts/setup-environment.sh || exit 1

make -C build install
