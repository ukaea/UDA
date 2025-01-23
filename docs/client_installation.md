---
layout: default
title: Installing a UDA client
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
past version 2.7.6 are available to pip install through pypi [here](https://pypi.org/project/uda/). Windows is supported 
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
python -m pip install uda
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
