# Universal Data Access (UDA)

The UDA API provides plugin driven data abstraction.

The UDA can be either run as a client-server API, running as thin client with all functionality being handled on a
remote server, or as fat-client API where both the client access and plugin functionality is run on the local machine.

## Getting UDA

UDA binaries can be downloaded from:

    <url>
    
The source code can be download from:

    <url>
    
The UDA git repository can be cloned:

    $ git clone ssh://git@git.iter.org/imas/uda.git -b develop
    
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

#### CentOS

Packages needed for CentOS

    $ LANG=C sudo yum -y groupinstall 'Development Tools'
    $ sudo yum -y install openssl-devel boost-devel swig-devel python-devel \
      postgresql-devel libxml2-devel gsl-devel libgcrypt-devel bzip2-devel \
      java-1.8.0-openjdk-devel
        
#### Ubuntu

    $ sudo apt-get install git python3-dev libssl-dev libboost-dev python3-numpy python3-matplotlib

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

On Linux system 

    make -C <build_dir> package

## Other Notes

Ninja installation for CentOS

    $ git clone https://github.com/ninja-build/ninja.git

    $ cd ninja
    $ ./configure.py --bootstrap
    $ export PATH="${HOME}/ninja:${PATH}"

Add the following to your .bashrc file:

    $ export PYTHONPATH=/usr/local/include
    
