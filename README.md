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
