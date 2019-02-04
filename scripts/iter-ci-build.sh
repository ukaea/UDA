#!/bin/bash
# Bamboo Build script
# Stage 2 : Build stage

# Set up environment for compilation
. scripts/setup-environment.sh || exit 1

make -C build
