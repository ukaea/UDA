#!/bin/sh -x

PYTHON_VERSION=3.5.0
PYTHON_MAINVERSION=${PYTHON_VERSION%.*}
SWIG_VERSION=3.0.0

BUILDROOT=${HOME}
BUILD_DIR=${BUILDROOT}/build
DOWNLOAD_DIR=${BUILDROOT}/download
STAGING_DIR=${BUILDROOT}/staging

set -e

#Initialize directories

install -d ${BUILD_DIR}
install -d ${STAGING_DIR}
install -d ${DOWNLOAD_DIR}

#Install swig

SWIG_SRC=swig-${SWIG_VERSION}.tar.gz
SWIG_DOWNLOAD="http://prdownloads.sourceforge.net/swig/swig-${SWIG_VERSION}.tar.gz"

if [ ! -f ${DOWNLOAD_DIR}/${SWIG_SRC} ]; then 
    wget  -O ${DOWNLOAD_DIR}/${SWIG_SRC} ${SWIG_DOWNLOAD}
fi

SWIG_SRC_DIR="${BUILD_DIR}/swig-${SWIG_VERSION}"
SWIG_INSTALL_DIR="${STAGING_DIR}"

if [ ! -e   ${SWIG_SRC_DIR}/.built ]; then
  rm -rf ${SWIG_SRC_DIR}
  cd ${BUILD_DIR}
  tar xzf ${DOWNLOAD_DIR}/${SWIG_SRC}
  cd ${SWIG_SRC_DIR}
  ./configure --prefix=${STAGING_DIR}
  make -j 8
  make install
  touch ${SWIG_SRC_DIR}/.built
fi

#Install python

PYTHON_SRC="Python-${PYTHON_VERSION}.tgz"
PYTHON_DOWNLOAD="https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz"

if [ ! -f ${DOWNLOAD_DIR}/${PYTHON_SRC} ]; then 
    wget  -O ${DOWNLOAD_DIR}/${PYTHON_SRC} ${PYTHON_DOWNLOAD}
fi

PYTHON_SRC_DIR="${BUILD_DIR}/Python-${PYTHON_VERSION}"
PYTHON_INSTALL_DIR="${STAGING_DIR}"

if [ ! -e   ${PYTHON_SRC_DIR}/.built ]; then
  rm -rf ${PYTHON_SRC_DIR}
  cd ${BUILD_DIR}
  tar xzf ${DOWNLOAD_DIR}/${PYTHON_SRC}
  cd ${PYTHON_SRC_DIR}
  ./configure --prefix=${STAGING_DIR} --enable-shared
  make -j ${MAKE_JOBS}
  make install
  #make altinstall DESTDIR="${STAGING_DIR}"
  touch ${PYTHON_SRC_DIR}/.built
fi
