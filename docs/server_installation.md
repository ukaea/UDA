---
layout: default
title: Server Installation
nav_order: 4
---

## Prerequisites

| Name              | Minimum Version          |
|-------------------|--------------------------|
| C++ compiler      | c++17 compliant compiler |
| CMake             | 3.0                      |
| Boost             | 1.65                     |
| OpenSSL version 1 | 1.1                      |
| pkg-config        | 0.29                     |
| libXML2           | 2.10                     |
| Capnproto         | 0.10                     |

## Supported OSs

The UDA server can been build on Linux and macOS. Note that the server will not run on Windows; only the client installation is supported. 

## Builing UDA

Note that the most up-to-date build script will be the one used for testing in the github CI tests [here](https://github.com/ukaea/UDA/blob/release/2.8.0/.github/workflows/cmake.yml). This will contain the relevant buld steps for Ubuntu and MacOS. There are also some dockerfiles available [here](https://github.com/ukaea/UDA/tree/release/2.8.0/docker) which will show the build steps for some other Linux flavours. 

**Clone the repo**

```bash
git clone git@github.com:ukaea/UDA.git
```

**Build configuration**

Cmake configuration options

|Option | Defaullt | Description |
|------|-----------|-------------|
|BUILD_SHARED_LIBS:BOOL | ON | Build shared libraries|
|CMAKE_INSTALL_PREFIX:PATH | /usr/local | Install path prefix, prepended onto install directories.|
|CMAKE_BUILD_TYPE:STRING | Debug | Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...|
|UDA_SERVER_HOST:STRING | `hostname` | define hostname in server configuration files|
|UDA_SERVER_PORT:STRING | 56565 | define port number in server configuration files|
|CLIENT_ONLY:BOOL | ON | Only build UDA client|
|SERVER_ONLY:BOOL | OFF | Only build UDA server|
|ENABLE_CAPNP:BOOL | ON | Enable Cap’n Proto serialisation|
|NO_MEMCACHE:BOOL | ON | Do not attempt to build with libmemcached support|
|NO_WRAPPERS:BOOL | OFF | Don't build any UDA client wrappers|
|NO_CLI:BOOL | OFF | Don't build UDA CLI|
|UDA_CLI_BOOST_STATIC:BOOL | OFF | compile commandline interface with static boost libraries|
|NO_CXX_WRAPPER:BOOL | OFF | Don't build C++ wrapper|
|NO_IDL_WRAPPER:BOOL | OFF | Don't build IDL wrapper|
|FAT_IDL:BOOL | OFF | Build IDL wrapper using fat-client|
|NO_JAVA_WRAPPER:BOOL | OFF | Don't build Java wrapper|
|NO_PYTHON_WRAPPER:BOOL | OFF | Don't build Python wrapper|



```bash
export UDA_ROOT=/usr/local
cmake -Bbuild -H. -DBUILD_SHARED_LIBS=ON -CMAKE_INSTALL_PREFIX=$UDA_ROOT -DCMAKE_BUILD_TYPE=Release
```

**Build**

```bash
cmake --build build
```

**Install**

```bash
cmake --install build
```
