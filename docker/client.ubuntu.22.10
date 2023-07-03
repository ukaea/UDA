FROM ubuntu:22.10

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y \
    git \
    libboost-dev \
    libboost-program-options-dev \
    libssl-dev \
    cmake \
    build-essential \
    pkg-config \
    libxml2-dev \
    libspdlog-dev \
    ninja-build \
    capnproto \
    libcapnp-dev \
    python3-dev \
    python3-pip \
    python3-venv

COPY .. /uda

RUN cd /uda && \
    cmake -G Ninja -B build \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON

RUN cd /uda && cmake --build build --config Release

RUN cd /uda && cmake --install build --config Release

RUN apt-get install -y python3-dev python3-pip python3-venv

RUN cp -r /usr/local/python_installer ./python_installer && \
    python3 -m venv ./venv && \
    source ./venv/bin/activate && \
    pip3 install Cython numpy && \
    pip3 install ./python_installer && \
    python3 -c 'import pyuda; print(pyuda.__version__)'
