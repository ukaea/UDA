---
layout: default
title: Installing a UDA client
---

TODO: Instructions for installing UDA client

To access data from any remote site hosting an UDA server you will only need to have the uda client installed on your local machine. The client-only build is supported on Linux, MacOS, and Windows. 

The uda client is a C library which exposes a number of API functions for requesting and unpacking data from a remote source. There are a number of client wrappers which provide an uda interface in different programming languages, such as Python and C++, often with a more user-friendly, object-oriented layer over the basic C-API.

In general the underlying uda client library must be fully built and then any number of additional language wrappers can be built on-top. With the exception on the python wrapper, building any additional wrappers is completely handled by the project CMake configuration. It's worth noting, however, that pre-built Docker images or python wheels may already be available for your platform. 

For python specifically, wheels are built for a range of architecure/OS/python-version combinations and all tagged releases past version 2.7.6 are available to pip install through pypi [here](https://pypi.org/project/uda/). Windows is supported from version 2.8.0.

The range of available dockerfiles are stored in the uda repository [here](https://github.com/ukaea/UDA/tree/main/docker), although no pre-built images are hosted publicly yet. 

Please raise an [issue](https://github.com/ukaea/UDA/issues) on the uda github repository for feature requests to support additional client wrapper langauges or additinal platforms for python wheels or docker images.  

## Installig the python pyuda client from pypi

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
Alternatively see the [CI scripts](https://github.com/ukaea/UDA/blob/main/.github/workflows/cmake.yml) in the uda repository for the latest build instructions used to test each release on ubuntu, MacOS, and Windows. The [docker files](https://github.com/ukaea/UDA/tree/main/docker) can also be a useful reference for other Linux flavours. 

### Requirements

### Ubuntu

Install dependencies from system package manager. 
```sh
sudo apt update && sudo apt install -y \
git \
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
cmake -G Ninja -B build \
    -DBUILD_SHARED_LIBS=ON \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON \
    -DCMAKE_INSTALL_PREFIX=install
```

Build and install.
```sh
cmake --build build -j --config Release --target install
```


### Alma


### Centos-7


### MacOS


### Windows (MSVC)


### Windows (MinGW)

```pwsh
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



### Testing client build using the uda CLI

```sh
cd <uda-install-dir>/bin
./uda_cli --host <host-ip> --port 56565 --request "help::help()" --source ""
```

## building the pyuda python wrapper
