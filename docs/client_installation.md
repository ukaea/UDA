---
layout: default
title: Client installation
nav_order: 3
---

# UDA client installation
{:.no_toc}

To access data from any remote site hosting an UDA server you will only need to have the UDA client installed on your local machine.
The client-only build is supported on Linux, MacOS, and Windows. 

The UDA client is a C library which exposes a number of API functions for requesting and unpacking data from a remote UDA server. 
There are a number of client wrappers which provide an UDA interface in different programming languages, such as Python and C++, 
often with a more user-friendly, object-oriented layer over the basic C-API.

In general, the underlying UDA client library must be fully built and then any number of additional language wrappers can 
be built on-top. With the exception of the python wrapper, building any additional wrappers is completely handled by the 
project CMake configuration. It's worth noting, however, that pre-built Docker images or python wheels may already be 
available for your platform. 

For python specifically, wheels are built for a range of architecure/OS/python-version combinations and all tagged releases 
past version 2.7.6 are available to pip install through pypi [here](https://pypi.org/project/pyuda/). Windows is supported 
from version 2.8.0.

The range of available dockerfiles are stored in the UDA repository [here](https://github.com/ukaea/UDA/tree/main/docker), 
although no pre-built images are hosted publicly yet. 

Please raise an [issue](https://github.com/ukaea/uda/issues) on the UDA github repository for feature requests to support 
additional client wrapper langauges or additinal platforms for python wheels or docker images.  

## Contents
{:.no_toc}
1. TOC 
{:toc}

## Installing the python pyuda client from pypi

```sh
python3 -m venv venv
source venv/bin/activate
python -m pip install --upgrade pip
python -m pip install pyuda
```

The python syntax to request a data item would then be:
```py
import pyuda
pyuda.Client.port = 56565 # or custom server port number
pyuda.Client.server = <your.uda.server.address>

client = pyuda.Client()
data_object = client.get('signal-name', source) 
```

## Building a docker image

```sh
wget https://github.com/ukaea/UDA/archive/refs/tags/2.7.6.tar.gz
tar -xzf 2.7.6.tar.gz
cd UDA-2.7.6/docker
docker build -t <tag-name> -f client.ubuntu.22.04.dockerfile
```

## Building the client from source

The following sections describe the installation procedure for the main operating systems supported.
Alternatively see the [CI scripts](https://github.com/ukaea/UDA/blob/main/.github/workflows/cmake.yml) in the UDA 
repository for the latest build instructions used to test each release on ubuntu, MacOS, and Windows. 
The [docker files](https://github.com/ukaea/UDA/tree/main/docker) can also be a useful reference for other Linux flavours. 

### Ubuntu

Install dependencies from system package manager. 
```sh
sudo apt update && sudo apt install -y \
git \
libtirpc-dev \
libboost-dev \
libboost-program-options-dev \
libssl-dev \
cmake \
build-essential \
libxml2-dev \
libspdlog-dev \
ninja-build \
capnproto \
libcapnp-dev \
python3-dev \
python3-pip \
python3-venv \
```

Configure the cmake project with desired options.
```sh
cmake -G Ninja -B build . \
    -DBUILD_SHARED_LIBS=ON \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON \
    -DCMAKE_INSTALL_PREFIX=install
```

Build and install
```sh
cmake --build build -j --config Release --target install
```

### Alma

```sh
  dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm && \
  dnf install -y \
      boost-devel \
      openssl-devel \
      libxml2-devel \
      libtirpc-devel \
      fmt fmt-devel \
      spdlog spdlog-devel \
      capnproto capnproto-devel
```

Configure the cmake project with desired options
```sh
cmake -B build . \
    -DBUILD_SHARED_LIBS=ON \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON \
    -DCMAKE_INSTALL_PREFIX=install
```

Build and install
```sh
cmake --build build -j --config Release --target install
```

### Centos-7

```sh
yum update -y &&
yum install -y wget openssl-devel libxml2-devel libtirpc-devel
```
Build additional dependencies from source
```sh
cd /tmp  

# libfmt
wget https://github.com/fmtlib/fmt/archive/refs/tags/10.0.0.tar.gz  
tar xzf 10.0.0.tar.gz  
cd fmt-10.0.0  
cmake -Bbuild -H. -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON  
cmake --build build -j --config Release --target install 

# spdlog
wget https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz  
tar xzf v1.11.0.tar.gz  
cd spdlog-1.11.0  
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON  
cmake --build build -j --config Release --target install 

# capnproto
wget https://github.com/capnproto/capnproto/archive/refs/tags/v0.10.4.tar.gz  
tar xzf v0.10.4.tar.gz  
cd capnproto-0.10.4  
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON  
cmake --build build  
cmake --install build 

# boost
wget https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_1_80_0.tar.gz 
tar xzf boost_1_80_0.tar.gz  
cd boost_1_80_0  
./bootstrap.sh --prefix=/usr/local  
./b2 --without-python --prefix=/usr/local install 
```

Configure the cmake project with desired options
```sh
cmake -B build . \
    -DBUILD_SHARED_LIBS=ON \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON \
    -DCMAKE_INSTALL_PREFIX=install
```

Build and install
```sh
cmake --build build -j --config Release --target install
```


### MacOS
install dependencies using homebrew

```sh
brew update-reset && brew install \
git \
boost \
openssl \
cmake \
pkg-config \
libxml2 \
spdlog \
ninja \
capnp \
```

Configure the cmake project with desired options
```sh
cmake -G Ninja -B build . \
    -DBUILD_SHARED_LIBS=ON \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON \
    -DCMAKE_INSTALL_PREFIX=install
```

Build and install
```sh
cmake --build build -j --config Release --target install
```

### Windows (MSVC)
install dependencies using vcpkg
```sh
vcpkg install --triplet x64-windows-static-md `
libxml2 `
capnproto `
boost-program-options `
boost-format `
boost-algorithm `
boost-multi-array `
openssl `
dlfcn-win32 `
spdlog 
```

build the xdr/rpc library bundled with uda for windows
```sh
cmake -Bextlib/build  ./extlib `
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
-DVCPKG_TARGET_TRIPLET="x64-windows-static-md" `
-DVCPKG_HOST_TRIPLET="x64-windows-static-md" `
-DCMAKE_GENERATOR_PLATFORM=x64 `
-DBUILD_SHARED_LIBS=OFF `
-DCMAKE_INSTALL_PREFIX=extlib/install

cmake --build extlib/build -j --config Release --target install
```

configure the cmake project
```sh
$Env:XDR_ROOT = 'extlib/install'
$Env:CMAKE_PREFIX_PATH = 'C:/vcpkg/installed/x64-windows-static-md'
$Env:Boost_DIR = 'C:/vcpkg/installed/x64-windows-static-md/share/boost'
$Env:LibXml2_DIR = 'C:/vcpkg/installed/x64-windows-static-md/share/libxml2'
$Env:CapnProto_DIR = 'C:/vcpkg/installed/x64-windows-static-md/share/capnproto'
$Env:fmt_DIR = 'C:/vcpkg/installed/x64-windows-static-md/share/fmt'

cmake -Bbuild . `
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
-DVCPKG_TARGET_TRIPLET="x64-windows-static-md" `
-DVCPKG_HOST_TRIPLET="x64-windows-static-md" `
-DCMAKE_GENERATOR_PLATFORM=x64 `
-DBUILD_SHARED_LIBS=ON `
-DSSLAUTHENTICATION=ON `
-DCLIENT_ONLY=ON `
-DENABLE_CAPNP=ON `
-DNO_JAVA_WRAPPER=ON `
-DNO_CXX_WRAPPER=ON `
-DNO_IDL_WRAPPER=ON `
-DNO_CLI=ON `
-DNO_MEMCACHE=ON `
-DCMAKE_INSTALL_PREFIX=install
```

Build and install
```sh
cmake --build build -j --config Release --target install
```

### Windows (MinGW)

Install dependencies using vcpkg
```sh
vcpkg install --triplet x64-mingw-static `
libxml2 `
capnproto `
boost-program-options `
boost-format `
boost-algorithm `
boost-multi-array `
openssl `
dlfcn-win32 `
spdlog `
```

build the xdr/rpc library bundled with uda for windows
```sh
cd extlib
cmake  -B build -G "MinGW Makefiles" . `
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
-DCMAKE_INSTALL_PREFIX=install
cmake --build build -j --config Release --target install

```

Configure the cmake project with desired options
```sh
$Env:XDR_ROOT = 'extlib/install'
cmake -B build . `
    -DBUILD_SHARED_LIBS=ON `
    -DSSLAUTHENTICATION=ON `
    -DCLIENT_ONLY=ON `
    -DENABLE_CAPNP=ON `
    -DCMAKE_INSTALL_PREFIX=install

```

Build and install
```sh
cmake --build build -j --config Release --target install
```



## Testing client build using the UDA CLI

```sh
cd <uda-install-dir>/bin
./uda_cli --host <host-ip> --port 56565 --request "help::help()" --source ""
```

## Consuming the installed client from CMake

CMake package exports are the primary supported interface for new integrations,
including native Windows builds. The package exports the existing C client API
through `UDA::client_static` and, when built, `UDA::client_shared`:

```cmake
find_package(UDA CONFIG REQUIRED COMPONENTS Client)
add_executable(my_client main.cpp)
target_link_libraries(my_client PRIVATE UDA::client_shared)
```

Use `UDA::client_static` explicitly for an embedded client, including the IDL
wrapper. Both targets provide installed include paths and dependency metadata.
Configure the consuming project with `-DCMAKE_PREFIX_PATH=/path/to/uda`, or set
`UDA_DIR` to the directory containing `UDAConfig.cmake`. Dependencies must be
installed for the same platform and toolchain; an export does not make binary
libraries portable between compilers or operating systems.

The installed-consumer check in `test/consumer` exercises this contract.
The `Client` component exports the core client. The standalone C++ wrapper
provides its own `UDACpp` package. Fat-client and plugin SDK exports are
separate follow-up work.

## Building the C++ wrapper against an installed client

Use wrapper sources from the same UDA release and a compatible C++ toolchain.
Point CMake at the core installation and install the wrapper to its own prefix
to avoid overwriting the in-tree wrapper libraries.

```sh
cmake -S source/wrappers/c++ -B build-cxx \
    -DCMAKE_PREFIX_PATH=/path/to/uda \
    -DCMAKE_INSTALL_PREFIX=/path/to/uda-cxx
cmake --build build-cxx
cmake --install build-cxx
```

The standalone build produces a static wrapper. Add `-DBUILD_SHARED_LIBS=ON`
to also build a shared wrapper when the installed core provides
`UDA::client_shared`. The fat C++ wrapper remains an in-tree build.

For downstream builds, add both prefixes to `CMAKE_PREFIX_PATH` and use the
installed wrapper target. It carries the matching core and Boost requirements.

```cmake
find_package(UDACpp CONFIG REQUIRED)
target_link_libraries(my_client PRIVATE UDA::cpp_static)
# Use UDA::cpp_shared if the shared wrapper was installed.
```

The standalone static archive contains only the C++ wrapper objects. Use the
CMake target to link the core archive as well. The legacy `uda-cpp.pc` continues
to describe the in-tree wrapper, whose archive includes the core objects.

## Legacy pkg-config integrations

Existing Unix integrations can continue to use `uda-client`, `uda-cpp`,
`uda-fat-client`, `uda-fat-cpp` and `uda-plugins`. Only modules whose libraries
are built are installed. They use package requirements for public libxml2/RPC
headers and private requirements for static-link dependencies such as OpenSSL
and fmt. Dependency `.pc` files must be available on the consuming machine.

```sh
export PKG_CONFIG_PATH=/path/to/uda/lib/pkgconfig
pkg-config --cflags --libs uda-client
pkg-config --static --cflags --libs uda-client
```

Use the installation's actual library directory (for example `lib64` or a
multiarch directory) in `PKG_CONFIG_PATH`. The UDA prefix is computed relative
to the metadata location so a prefix-local installation can be moved.

`--static` includes private dependencies; it does not force the linker to use
an archive if shared libraries are also installed. Use a C++ linker driver for
static UDA clients, including callers written in C, because the implementation
uses C++. C++ wrapper consumers also need Boost headers in their toolchain's
include search path.

`UDA_INSTALL_PKGCONFIG` defaults to `ON` on Unix and `OFF` on Windows. Windows
integrations should use the CMake exports; the legacy metadata does not claim
MSVC or MinGW portability. To omit compatibility metadata on Unix, configure
with `-DUDA_INSTALL_PKGCONFIG=OFF`. This does not affect the CMake exports.

See `test/pkgconfig` for independent shared/static consumer checks.

## Building the pyuda python wrapper

After successful client library build there will be a directory called `python_installer` from which you can pip install pyuda, this command will also compile the cython part of the pyuda module.

```sh
cd <uda-install-dir>/python_installer
python3 -m venv venv
source venv/bin/activate
python3 -m pip install --upgrade pip wheel
python3 -m pip install .

# test
python3 -c 'import pyuda; print(pyuda.__version__)'
```

## Building the IDL wrapper against an installed client

The IDL wrapper can be configured separately after the UDA client libraries
have been installed. `CMAKE_PREFIX_PATH` identifies the UDA installation and
`IDL_ROOT` identifies the IDL installation:

```sh
cmake -S source/wrappers/idl -B build-idl \
    -DCMAKE_PREFIX_PATH=<uda-install-dir> \
    -DIDL_ROOT=<idl-install-dir> \
    -DCMAKE_INSTALL_PREFIX=<uda-idl-install-dir>

cmake --build build-idl
cmake --install build-idl
```

The standalone wrapper links the exported `UDA::client_static` target. A fat
IDL client still requires the full in-tree UDA build.

Use the SDK for the target platform and architecture. `FindIDL.cmake` reads
the version from `external/include/idl_export.h` and searches the corresponding
`bin/bin.linux.x86_64`, `bin/bin.darwin.arm64`, `bin/bin.darwin.x86_64`, or
Windows library directory. Configuration and compilation do not start IDL.
On macOS, select a single `CMAKE_OSX_ARCHITECTURES` value when cross-compiling;
the installed UDA client and its dependencies must have the same architecture.
The macOS DLM is installed as `uda.so`, following the vendor's DLM examples.

GNU/Linux builds retain the existing static-libstdc++ workaround. Install the
compiler's static C++ runtime, or set `STDCXX_STATIC` to its `libstdc++.a`.
Apple Clang builds use libc++ instead. Static linkage does not establish
isolation from a C++ runtime already loaded by the interpreter; compatibility
with the deployed IDL host still requires a separate runtime check.

The obsolete `CALLIDAM /TEST` keyword reports an unsupported-operation error;
the core API it previously called no longer exists. Normal requests use the
existing client APIs. Linux DLM links reject unresolved symbols at build time.

# Configuring an authenticated client connection

Once you have a certificate bundle, set the following environment variables to the individual SSL file locations. See the [Authentication section](/UDA/authentication) for more details about authenticated connections in uda.

These variables will need to be set before every new client session when you are connecting to an SSL-authenticated uda server.

```sh
SSL_HOME="<certificate_install_dir>/.uda"

export UDA_CLIENT_SSL_KEY="${SSL_HOME}/keys/<username>.key.pem"
export UDA_CLIENT_CA_SSL_CERT="${SSL_HOME}/certs/uda-ca.cert.pem"
export UDA_CLIENT_SSL_CERT="${SSL_HOME}/certs/<username>.cert.pem"
export UDA_CLIENT_SSL_AUTHENTICATE=1

```
