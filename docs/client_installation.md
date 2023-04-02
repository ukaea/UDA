---
layout: default
title: Installing a UDA client
---

TODO: Instructions for installing UDA client

### Installing on MacOS

To build UDA on macOS you'll need the homebrew package manager. You can find
instructions for installing homebrew [here](https://brew.sh/).

With homebrew installed you can install the dependencies as follows:

```bash
brew update && brew install \
    git \
    boost \
    openssl \
    cmake \
    pkg-config \
    libxml2 \
    spdlog \
    ninja \
    capnp
```

Then UDA can be built and installed as follows:

```bash
cmake -G Ninja -Bbuild -H. \
    -DBUILD_SHARED_LIBS=ON \
    -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3
cmake --build build
cmake --install build
```