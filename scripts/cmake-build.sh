#!/bin/bash
# ============================================================================
# This is a template demonstrating how to structure a CMake build script.
# Modify the UDA_ROOT and CMake options as needed for your specific project.
# ============================================================================

UDA_ROOT="</usr/local>"     # Root directory where UDA will install and server run directory

# Configuration Options
BUILD_TYPE="Release"        # Options: Debug, Release, RelWithDebInfo, MinSizeRel
BUILD_DIR="build"           # Name of the build directory
INSTALL_DIR=$UDA_ROOT       # Installation directory

# UDA Options
# ENABLE_TESTS=ON           # To be added later (if ENABLE_TESTS add_subdirectory(tests))
BUILD_SHARED_LIBS=ON        # Build shared libraries                                                (Default ON)
ENABLE_CAPNP=ON             # Enable Cap'n Proto serialisation                                      (Default ON)
CLIENT_ONLY=ON              # Only build UDA client                                                 (Default ON)
SERVER_ONLY=ON              # Only build UDA server                                                 (Default OFF)

# Server Options
UDA_SERVER_HOST="localhost" # Define hostname in server configuration files                         (Default `hostname`)
UDA_SERVER_PORT="56565"     # Define port number in server configuration files                      (Default 56565)

# CLI
NO_CLI=OFF                  # Don't build UDA CLI                                                   (Default OFF)
UDA_CLI_BOOST_STATIC=OFF    # Compile CLI with static Boost libraries                               (Default OFF)

# Wrappers
NO_WRAPPERS=OFF             # Don't build any UDA client wrappers                                   (Default OFF)
NO_CXX_WRAPPER=OFF          # Don't build C++ wrapper                                               (Default OFF)
NO_IDL_WRAPPER=OFF          # Don't build IDL wrapper                                               (Default OFF)
NO_PYTHON_WRAPPER=OFF       # Don't build Python wrapper                                            (Default OFF)
FAT_IDL=OFF                 # Build IDL wrapper using fat-client                                    (Default OFF)

# Memcache
NO_MEMCACHE=ON              # Do not attempt to build wth libmemcached support                      (Default ON)

# Run CMake Configuration
echo "Running CMake configuration..."
cmake -B $BUILD_DIR -H. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS \
    -DENABLE_CAPNP=$ENABLE_CAPNP \
    -DCLIENT_ONLY=$CLIENT_ONLY \
    -DSERVER_ONLY=$SERVER_ONLY \
    -DUDA_SERVER_HOST=$UDA_SERVER_HOST \
    -DUDA_SERVER_PORT=$UDA_SERVER_PORT \
    -DNO_CLI=$NO_CLI \
    -DUDA_CLI_BOOST_STATIC=$UDA_CLI_BOOST_STATIC \
    -DNO_WRAPPERS=$NO_WRAPPERS \
    -DNO_CXX_WRAPPER=$NO_CXX_WRAPPER \
    -DNO_IDL_WRAPPER=$NO_IDL_WRAPPER \
    -DNO_PYTHON_WRAPPER=$NO_PYTHON_WRAPPER \
    -DFAT_IDL=$FAT_IDL \
    -DNO_MEMCACHE=$NO_MEMCACHE

# Run CMake Build
echo "Building the project..."
cmake --build $BUILD_DIR "$@"

# Optional: Install if Needed
echo "Installing to $INSTALL_DIR..."
cmake --install $BUILD_DIR
