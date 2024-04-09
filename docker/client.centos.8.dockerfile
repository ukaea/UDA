FROM centos:8

SHELL ["/bin/bash", "-c"]

RUN sed -i 's/$releasever/$releasever-stream/g' /etc/yum.repos.d/CentOS-*
RUN yum update -y --allowerasing && \
    yum group install -y 'Development Tools' && \
    yum install -y \
        git \
        wget \
        boost-devel \
        openssl-devel \
        cmake \
        libxml2-devel \
        libtirpc-devel \
        python39-devel \
        python39-pip

# Building libfmt from source
RUN cd /tmp && \
    wget https://github.com/fmtlib/fmt/archive/refs/tags/10.0.0.tar.gz && \
    tar xzf 10.0.0.tar.gz && \
    cd fmt-10.0.0 && \
    cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON && \
    cmake --build build -j && \
    cmake --install build

# Building spdlog from source
RUN cd /tmp && \
    wget https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz && \
    tar xzf v1.11.0.tar.gz && \
    cd spdlog-1.11.0 && \
    cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON && \
    cmake --build build -j && \
    cmake --install build

RUN cd /tmp && \
    wget https://github.com/capnproto/capnproto/archive/refs/tags/v0.10.4.tar.gz && \
    tar xzf v0.10.4.tar.gz && \
    cd capnproto-0.10.4 && \
    cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON && \
    cmake --build build -j && \
    cmake --install build

COPY . /uda

RUN cd /uda && \
    cmake -B build \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DSSLAUTHENTICATION=ON \
    -DCLIENT_ONLY=ON \
    -DENABLE_CAPNP=ON

# LD_LIBRARY_PATH needed by capnp scheam generator binary
RUN cd /uda && LD_LIBRARY_PATH=/usr/local/lib64 cmake --build build --config Release

RUN cd /uda && cmake --install build --config Release

RUN cp -r /usr/local/python_installer ./python_installer && \
    export LD_LIBRARY_PATH=/usr/local/lib64 && \
    python3 -m venv ./venv && \
    source ./venv/bin/activate && \
    pip3 install Cython numpy && \
    pip3 install ./python_installer && \
    python3 -c 'import pyuda; print(pyuda.__version__)'

