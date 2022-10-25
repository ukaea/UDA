---
layout: default
title: Server Installation
---

## Prerequisites

| Name              | Minimum Version          |
|-------------------|--------------------------|
| C++ compiler      | c++11 compliant compiler |
| CMake             | 3.0                      |
| Boost             | 1.65                     |
| OpenSSL version 1 | 1.1                      |
| pkg-config        | 0.29                     |
| libXML2           | 2.10                     |
 | Capnproto         | 0.10                     |

## Supported OSs

UDA has been build on Linux, macOS and Windows.

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