#!/bin/bash
# Bamboo Build script
# Stage 4 : Package stage

# Set up environment for compilation
source ./scripts/iter-ci-setup-env.sh || exit 1

make -C build package
