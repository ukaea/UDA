#!/bin/bash
# Bamboo Build script
# Stage 3 : Install stage

# Set up environment for compilation
source ./scripts/iter-ci-setup-env.sh || exit 1

make -C build install
