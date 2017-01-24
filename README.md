IDAM installation on linux (CentOS) and OSX
==============

Clone the repository

    $ git clone ssh://git@git.iter.org/imas/idam.git -b ccfe/build

Packages needed for CentOS 

    $ LANG=C sudo yum -y groupinstall 'Development Tools'
    $ sudo yum -y install openssl-devel boost-devel swig-devel python-devel \
      postgresql-devel libxml2-devel gsl-devel   libgcrypt-devel bzip2-devel \
        java-1.8.0-openjdk-devel

Ninja installation for CentOS

    $ git clone https://github.com/ninja-build/ninja.git

    $ cd ninja
    $ ./configure.py --bootstrap
    $ export PATH="${HOME}/ninja:${PATH}"

Clone the repository from the ccfe/build branch

    $ git clone ssh://git@git.iter.org/imas/idam.git -b ccfe/build
    $ cd idam

## Installing client on LINUX

    $ cmake -Bbuild_linux -Hlatest/source -DTARGET_TYPE=MAST \
      -DCMAKE_MAKE_PROGRAM=${HOME}/ninja/ninja -GNinja \
        -DCMAKE_INSTALL_PREFIX=idam_linux
    $ ninja -C build_linux install

## Installing server on LINUX

    $ cmake -Bbuild_linux -Hlatest/source -DTARGET_TYPE=MAST -GNinja \
      -DCMAKE_MAKE_PROGRAM=${HOME}/ninja/ninja \
        -DCMAKE_INSTALL_PREFIX=idam_linux  -DCLIENT_ONLY=OFF
    $ ninja -C build_linux install
    $ export LD_LIBRARY_PATH={HOME}/idam/idam_linux/lib:$LD_LIBRARY_PATH


## Installing client on OSX

    $ cmake -Bbuild_osx -Hlatest/source -DTARGET_TYPE=MAST -GNinja \
    -DCMAKE_INSTALL_PREFIX=idam_osx  && ninja -C build_osx install

## Installing server on OSX

    $ cmake -Bbuild_osx -Hlatest/source -DTARGET_TYPE=MAST -GNinja \
      -DCMAKE_INSTALL_PREFIX=idam_osx  -DCLIENT_ONLY=OFF
    $ ninja -C build_osx install
    $ install_name_tool -change libidamcpp.1.dylib ${HOME}/idam/idam_osx/lib/libidamcpp.1.dylib _cidam.so
    $ install_name_tool -change libidam64.1.dylib ${HOME}/idam/idam_osx/lib/libidam64.1.dylib _cidam.so
    $ install_name_tool -change libidam64.1.dylib ${HOME}/idam/idam_osx/lib/libidam64.1.dylib libidamcpp.1.dylib
    
 
## Installing client on Ubuntu 16.04.1 LTS

    $ sudo apt-get install git python3-dev libssl-dev libboost-dev python3-numpy python3-matplotlib
    $ git clone git@git.ccfe.ac.uk:data-and-codeing/idam.git
    $ cd idam/latest/source
    $ cmake -Bbuild -DTARGET_TYPE=OTHER -DCLIENT_ONLY=ON -H.
    $ sudo make -C build install

Add the following to your .bashrc file:

    $ export PYTHONPATH=/usr/local/include