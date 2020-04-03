# Universal Data Access (UDA)

[![pipeline status](https://git.ccfe.ac.uk/MAST-U/UDA/badges/develop/pipeline.svg)](https://git.ccfe.ac.uk/MAST-U/UDA/commits/develop)
[![coverage report](https://git.ccfe.ac.uk/MAST-U/UDA/badges/develop/coverage.svg)](https://git.ccfe.ac.uk/MAST-U/UDA/commits/develop)

The UDA API provides plugin driven data abstraction.

The UDA can be either run as a client-server API, running as thin client with all functionality being handled on a
remote server, or as fat-client API where both the client access and plugin functionality is run on the local machine.

## Licence

See LICENCE.txt for details.

## Getting UDA

UDA binaries can be downloaded from:

    <url>

The source code can be download from:

    <url>

The UDA git repository can be cloned:

    git clone ssh://git@git.iter.org/imas/uda.git -b develop

## Building from source

### Dependencies

UDA requires the following to avail in order to build:

| Name | Version | Required For |
| --- | --- | --- |
| cmake | \> 3.0 | |
| OpenSSL | | |
| PostgreSQL | | |
| LibXml2 | | |
| LibMemcached | | to enable caching |
| swig  | 3.0.0 | Python & HTTP wrappers |
| python | \> 3.0 | Python & HTTP wrappers |
| boost | | C++, Python & HTTP wrappers |
| java | | Java wrapper |
| hdf5 |  | hdf5 plugin |
| netcdf | | netcdf plugin |
| MDSplus | | MDS+ plugin |

#### Windows

> Note: If you want to use Visual Studio 2019 to compile UDA, please refer to sections [Visual Studio](#visual-studio) and [vcpkg](#vcpkg) below

Building extlibs (running in Powershell):

    cd extlib
    mkdir build
    cd build
    cmake.exe .. -G"MinGW Makefiles" -DBUILD_SHARED_LIBS=ON
    mingw32-make.exe
    cd ..
    .\install.bat

Building extlibs (running in MinGW64 Shell):

    cd extlib/portablexdr-4.9.1
    ./configure
    make
    cd ..
    ./install.sh

Building extlibs (running in VS2019 x64 Native Tools):

    cd extlib
    mkdir build
    cd build
    cmake.exe .. -G"Visual Studio 16 2019"
    msbuild.exe ALL_BUILD.vcxproj /p:configuration=release /p:platform=x64
    cd ..
    install.bat

Tested and built on Windows 10 (built using MinGW 64-bit, running in Powershell):

    mkdir build
    cd build
    ..\scripts\cmake-win.bat
    mingw32-make.exe
    mingw32-make.exe install

Tested and built on Windows 10 (built using MinGW 64-bit, running in MinGW64 Shell):

    mkdir build
    cd build
    cmake .. -G"Unix Makefiles" -DBUILD_SHARED_LIBS=ON -DTARGET_TYPE=OTHER
    make
    make install

Tested and built on Windows 10 (built using VS2019 x64 Native Tools):

    mkdir build
    cd build
    cmake.exe .. -G"Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DNO_MODULES=ON -DTARGET_TYPE=OTHER -DBUILD_SHARED_LIBS=ON
    msbuild ALL_BUILD.vcxproj /p:configuration=release /p:platform=x64
    msbuild INSTALL.vcxproj /p:configuration=release /p:platform=x64

Running Python client:

    $python_dir = (Get-Item (Get-Command python).Source).DirectoryName
    rm $python_dir\Lib\site-packages\pyuda
    copy -Recurse .\include\pyuda  $python_dir\Lib\site-packages\
    cp .\extlib\lib\libxdr.dll $python_dir\Lib\site-packages\pyuda\
    
    Set-Item -Path env:UDA_HOST -Value "idam3.mast.ccfe.ac.uk"
    Set-Item -Path env:UDA_HOST -Value "56565"
    python

#### CentOS

Packages needed for CentOS

    $ LANG=C sudo yum -y groupinstall 'Development Tools'
    $ sudo yum -y install openssl-devel boost-devel swig-devel python-devel \
      postgresql-devel libxml2-devel gsl-devel libgcrypt-devel bzip2-devel \
      java-1.8.0-openjdk-devel

#### Ubuntu

    sudo apt-get install git python3-dev libssl-dev libboost-dev python3-numpy python3-matplotlib

#### OSX

### Running cmake configuration

To configure the UDA build you first need to run cmake:

    cmake -B<build_dir> -H. -DTARGET_TYPE:STRING=<target>

Where `<build_dir>` is the build directory to create and `<target>` is the target specific configuration to use. The
different targets available are `MAST`, `ITER` and `OTHER`. These are available in the `cmake/Config` directory with
the file name `target-<target>.cmake`. To add a new target simply copy one of these files and rename to the desired target name.

An example configuration command is:

    cmake -Bbuild -H. -DTARGET_TYPE:STRING=OTHER

By default UDA will configure to build in client-server mode with both the client and server being built.

To only build the client use:

    cmake -DCLIENT_ONLY:BOOL=TRUE ...

To build UDA in fat-client mode use:

    cmake -DFAT_BUILD:BOOL=TRUE ...

### Building

    make -C <build_dir>

### Installing

    make -C <build_dir> install

### Packaging

On Linux system:

    make -C <build_dir> package

On Windows system (MinGW):

    make -C <build_dir> package

On Windows system (VS2019):

    msbuild.exe INSTALL.vcxproj /p:configuration=release /p:platform=x64

## Visual Studio

UDA can be compiled with Visaul Studio 2019.
To do that, Vidual Studio need to be iunstalled witrh at least the following packages:

- C++ Desktop development tools
- CMake tools
- Python 3.7
- MFC and ATL libraries
- English language pack (even if you choose another language)
- Windows 10 SDK v10.0.17134.0

## vcpkg

vcpkg if a library manager designed by Microsoft to procure standard libraries for Visual Studio.

It supports CMake toolchain, easily usable with UDA CMakeList.txt files.

To use vcpkg, follow theses steps:

    git clone https://github.com/Microsoft/vcpkg
    cd vcpkg
    bootstrap-vcpkg.bat

After that, vcpkg tool is ready to acquire libraries.
For UDA, severals libraries are mandatory, the following command download, compile and install them :

    vcpkg install libxml2:x64-windows openssl:x64-windows boost:x64-windows python3:x64-windows dlfcn-win32:x64-windows libpq:x64-windows netcdf-c:x64-windows blitz:x64-windows

## Other Notes

Ninja installation for CentOS:

    git clone https://github.com/ninja-build/ninja.git

    cd ninja
    ./configure.py --bootstrap
    export PATH="${HOME}/ninja:${PATH}"

Add the following to your .bashrc file:

    export PYTHONPATH=/usr/local/include
