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

**Clone the repo**

```bash
git clone git@github.com:ukaea/UDA.git
```

**Build configuration**

Cmake configuration options

| Options              | Description |
|----------------------|-------------|
| BUILD_SHARED_LIBS    |             |
| OPENSSL_ROOT_DIR     |             |
| BOOST_ROOT           |             |
| CMAKE_INSTALL_PREFIX |             |
| CMAKE_BUILD_TYPE     |             |

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
