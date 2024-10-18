# Universal Data Access (UDA)

![Build Workflow](https://github.com/ukaea/UDA/actions/workflows/cmake.yml/badge.svg)

The UDA API provides plugin driven data abstraction.

The UDA can be either run as a client-server API, running as thin client with all functionality being handled on a
remote server, or as fat-client API where both the client access and plugin functionality is run on the local machine.

## Licence

See LICENCE.txt for details.

## Getting UDA

The source code can be downloaded from:

    https://github.com/ukaea/UDA/releases/

The UDA git repository can be cloned:

    git clone git@github.com:ukaea/UDA.git

## Getting the UDA client

The easiest way to obtain the client is to pip install the python wrapper (pyuda), wheels are uploaded for every tagged release from version 2.7.6. Further details are available on [pypi](https://pypi.org/project/uda/).

```sh
pip install uda
```

For any other use cases please see the documentation to build from source [here](https://ukaea.github.io/UDA/client_installation/).

## Building UDA Server from source

### Dependencies

UDA requires the following to avail in order to build:

| Name | Version | Required For |
| --- | --- | --- |
| cmake | \> 3.0 | |
| OpenSSL | | |
| LibXml2 | | |
| libfmt | | |
| spdlog | | |
| capnproto | | |
| boost | | C++, Python & HTTP wrappers |
| LibMemcached | | to enable caching |
| python | \> 3.0 | Python wrapper |

An example installation for ubuntu 22.10 would be as follows.

Start by install all system-level dependencies.
```sh
sudo apt update && sudo apt install -y
git
libboost-dev
libboost-program-options-dev
libssl-dev
cmake
build-essential
pkg-config
libxml2-dev
libspdlog-dev
ninja-build
capnproto
libcapnp-dev
python3-dev
python3-pip
python3-venv
```

Configure the cmake project.
```sh
cmake -G Ninja -B build . \
-DBUILD_SHARED_LIBS=ON \
-DSSLAUTHENTICATION=ON \
-DCLIENT_ONLY=OFF \
-DENABLE_CAPNP=ON \
-DCMAKE_INSTALL_PREFIX=install
```

```sh
cmake --build build -j --config Release --target install
```

