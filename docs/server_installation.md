---
layout: default
title: Server Installation
nav_order: 4
---

# UDA server installation
{:.no_toc}

## Contents
{:.no_toc}
1. TOC 
{:toc}


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

## Building UDA

Note that the most up-to-date build script will be the one used for testing in the github CI tests [here](https://github.com/ukaea/UDA/blob/release/2.8.0/.github/workflows/cmake.yml). This will contain the relevant buld steps for Ubuntu and MacOS. There are also some dockerfiles available [here](https://github.com/ukaea/UDA/tree/release/2.8.0/docker) which will show the build steps for some other Linux flavours. 

**Clone the repo**

```bash
git clone git@github.com:ukaea/UDA.git
```

**Build configuration**

Cmake configuration options

|Option | Defaullt | Description |
|------|-----------|-------------|
|BUILD_SHARED_LIBS:BOOL | ON | Build shared libraries|
|CMAKE_INSTALL_PREFIX:PATH | /usr/local | Install path prefix, prepended onto install directories.|
|CMAKE_BUILD_TYPE:STRING | Debug | Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...|
|UDA_SERVER_HOST:STRING | `hostname` | define hostname in server configuration files|
|UDA_SERVER_PORT:STRING | 56565 | define port number in server configuration files|
|CLIENT_ONLY:BOOL | ON | Only build UDA client|
|SERVER_ONLY:BOOL | OFF | Only build UDA server|
|ENABLE_CAPNP:BOOL | ON | Enable Cap’n Proto serialisation|
|NO_MEMCACHE:BOOL | ON | Do not attempt to build with libmemcached support|
|NO_WRAPPERS:BOOL | OFF | Don't build any UDA client wrappers|
|NO_CLI:BOOL | OFF | Don't build UDA CLI|
|UDA_CLI_BOOST_STATIC:BOOL | OFF | compile commandline interface with static boost libraries|
|NO_CXX_WRAPPER:BOOL | OFF | Don't build C++ wrapper|
|NO_IDL_WRAPPER:BOOL | OFF | Don't build IDL wrapper|
|FAT_IDL:BOOL | OFF | Build IDL wrapper using fat-client|
|NO_JAVA_WRAPPER:BOOL | OFF | Don't build Java wrapper|
|NO_PYTHON_WRAPPER:BOOL | OFF | Don't build Python wrapper|



```bash
export UDA_ROOT=/usr/local
cmake -Bbuild -H. -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$UDA_ROOT -DCMAKE_BUILD_TYPE=Release
```

**Build**

```bash
cmake --build build
```

**Install**

```bash
cmake --install build
```

## Running the server

There are currently two main options to run the server as a system service, either using xinetd or systemd as super-server daemons. It's also possible to run a test server manually using xinetd, but this is more for testing and debugging and will be covered in the [development](/UDA/development) section of the docs. 

Note that concurrent connections are managed by spinning up a new instance of the server process for each new client process. All the listener options listed here will have configurable options to control how many concurrent connections can be permitted at any one time, both as a total number of connections and the acceptable number from any single source (IP address).

The server process launched for each request is the `udaserver.sh` script in the `etc` subdirectory. This script loads a companion configuration file `udaserver.cfg` and then launches the uda server binary (`/bin/uda_server`).

Specifying the port number on which to listen for new connection requests is also done in the configuration file for the system service; the details will be covered for each specific listener below. 

### systemd

The systemd files will be automatically configured as part of the uda server build process and will be installed to the `etc` subdirectory in the uda install location. Note that the maximum connections options are set in the `uda.socket` file and the path to the script for launching a new server is set in the `uda@.service` file.

Setting the port number is done in the `uda.socket` file in the Socket.ListenStream field.

Finally, the service can be started as follows:

```sh
sudo cp <install_dir>/etc/uda.socket <install_dir>l/etc/uda@.service /etc/systemd/system
sudo systemctl start uda.socket
sudo systemctl enable uda.socket
```

### xinetd
The xinetd configuration file `xinetd.conf` will be installed to the `etc` subdirectory in the uda install location. This file can be used to set the maximum connection limits and the port number.

Copy this configuration file into `/etc/xinetd.d` and start (or restart) the xinetd system service to deploy. It can be a good idea to rename it to something more descriptive when moving, e.g. `uda` instad of `xinetd.conf`.

```sh
cp <install_dir>/etc/xinetd.conf /etc/xinetd.d/uda
sudo systemctl start xinetd
sudo systemctl enable xinetd
```

### launchd
The launchd configuration file `launchd.udaserver.plist` will be installed to the etc subdirectory in the uda install location. The `SockServiceName` key in this file defines what the port number will be (56565 by default). 

Copy the plist file to the MacOS LaunchAgents directory and start the super-server as shown below. 

```sh
cp <install_dir>/etc/launchd.udaserver.plist ~/Library/LaunchAgents/.
launchctl load ~/Library/LaunchAgents/launchd.udaserver.plist
launchctl start udaserver
```

You can stop as follows:

```sh
launchctl stop udaserver
launchctl unload ~/Library/LaunchAgents/launchd.udaserver.plist
```


## Server configuration and logging

Currently, configuration options are loaded as environment variables in the uda server subprocess environment before the server binary is launched. There are 3 main categories of configuration files to be aware of. 

### Machine-specific configurations 

There is a directory in `<install_dir>/etc` called `machine.d` in which general configuration options for a specific domain can be set. The uda server will look for a file named after what the `dnsdomainname` command evaluates to on the current machine. This can be used to e.g. load environment modules on an HPC system or shared cluster. 

### General server-specific options 

Most general server options are set in `<install_dir>/etc/udaserver.cfg` including the debug level, the file locations of plugin-registration files, and to add the library locations of uda and any uda-plugin builds (as well as any other dependencies such as imas) to the LD_LIBRARY_PATH. 

Note that the UDA_LOG_LEVEL should Generally be set to ERROR for a production deployment. The DEBUG level will impose severe a performance penalty, but can be used to print some very verbose logs for each incoming request during server development.

##### UDA_ALLOWED_PATHS
This environment variable defines the permitted file paths for the UDA server. By default, all file access is prohibited if this is unset or empty. Multiple paths can be specified using semi-colons (`;`). Setting the path to `/` allows unrestricted access.

**Example**
```sh
export UDA_ALLOWED_PATHS=/
```
**Security Note**  
Restrict the paths as narrowly as possible to limit server file access and reduce risk.

### Plugin-specific options

Any configuration options for specific plugins will be stored in different files for each plugin in the `<install_dir>/etc/plugins.d` directory. This may be to set file paths for additional configuration files required by a plugin at runtime, to set a list of permissible filesystem locations a plugin is permitted to access (e.g. in the  bytes plugin), or to set memory limits on the size of data a plugin is allowed to cache. 

Refer to any plugin-specific documentation for more information on configuration options for a particular plugin.

## Configuring a server to accept authenticated connections only
UDA currently only supports X509 based SSL certificate authentication, see the [authentication](/UDA/authentication) section for more details.

```sh
export UDA_SERVER_SSL_AUTHENTICATE=1
export UDA_SERVER_SSL_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.pem"
export UDA_SERVER_SSL_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
export UDA_SERVER_CA_SSL_CERT="${UDA_ROOT}/etc/.uda/certs/uda-ca.cert.pem"
export UDA_SERVER_CA_SSL_CRL="${UDA_ROOT}/etc/.uda/crl/uda-ca.crl.pem"
```
