---
layout: default
title: Server Installation
has_children: true
nav_order: 2
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
|UDA_INSTALL_LAYOUT:STRING | LEGACY | Installation layout. `LEGACY` preserves the prefix-local UDA tree; `SYSTEM` uses GNU system directories and places configuration under `/etc/uda`.|
|UDA_INSTALL_PKGCONFIG:BOOL | ON on Unix; OFF on Windows | Install legacy Unix pkg-config compatibility metadata. CMake package exports are always installed.|
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

For a system installation or a staged RPM/DEB filesystem, select the system
layout explicitly:

```bash
cmake -Bbuild -H. \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DUDA_INSTALL_LAYOUT=SYSTEM \
    -DCMAKE_BUILD_TYPE=Release
```

Selecting the layout only controls install destinations. It does not select a
package generator or otherwise change the build workflow.

`SYSTEM` honors `CMAKE_INSTALL_LIBDIR` (including `lib64` and multiarch paths).
Systemd units use the separate `UDA_INSTALL_SYSTEMD_UNITDIR`, defaulting to
`lib/systemd/system` relative to the install prefix. Set it to an absolute or
prefix-relative path when a distribution requires a different location.
The plugin helper accepts `--config-dir` to override its configured plugin
configuration directory.

The service templates still require deployment configuration: select an
appropriate service user and provision writable log/runtime directories before
starting the server. Layout selection does not create users or set ownership.
With prefix `/usr`, configuration is under `/etc/uda`, scripts under
`/usr/libexec/uda`, and logs under `/var/log/uda`. The runtime directory follows
GNUInstallDirs' `CMAKE_INSTALL_RUNSTATEDIR`.

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

##### UDA_DOWNLOADABLE_PATHS
This environment variable lists the files that clients may download through plugins that return file contents,
such as `bytes::read` and `bytes::size`. A requested path is allowed if it is one of the listed paths or lies
beneath one. Separate multiple paths with colons (`:`), as for `PATH` and the Landlock lists (on Windows, use
semicolons). If the variable is unset or empty, every request is rejected.

Requested paths and list entries are both resolved to their real location first, so symbolic links and `..`
cannot be used to escape the list, and a listed symbolic link refers to its target. Matching is by whole path
components: `/data` allows `/data/shot1` but not `/data2`. Entries must be absolute and must exist; empty,
relative and missing entries are ignored and logged as warnings.

On Linux and macOS, the `bytes` plugin opens the file first and then checks where the opened file actually is,
so a symbolic link swapped in during the request cannot redirect it. Only regular files are served; directories,
FIFOs and devices are rejected. Plugin authors should use `open_downloadable_file` from `plugins/utils.h` rather
than checking a name with `check_allowed_path` and opening it afterwards.

**Example**
```sh
export UDA_DOWNLOADABLE_PATHS="/srv/uda/data:/common/uda-scratch"
```

`UDA_ALLOWED_PATHS` is the older name for this variable and still works. It is used only when
`UDA_DOWNLOADABLE_PATHS` is unset or empty; if both are set, `UDA_ALLOWED_PATHS` is ignored and a warning is
logged. `UDA_ALLOWED_PATHS` keeps its original semicolon separator, so existing configurations work unchanged;
when renaming it to `UDA_DOWNLOADABLE_PATHS`, also change the separators to colons. A `;` left in
`UDA_DOWNLOADABLE_PATHS` is logged as a warning, and the entry it is part of matches nothing.

**Security Note**  
This controls what clients can *request*, which is different from what the server process can *open*. The
server must be able to read its own configuration, TLS certificates and private key, and system files, so do
not use `/` or any directory containing those. List only data directories. On Linux, the
[Landlock policy](/UDA/server_sandboxing) separately limits what the server process can open; it does not
replace this list.

### Linux filesystem restrictions with Landlock

UDA supports optional kernel-enforced filesystem restrictions through Landlock.
Build with `-DENABLE_LANDLOCK=ON`. A Landlock build then enforces a policy that fails
closed: the server can only open paths in its read-only, read/write and execute
lists, and the shipped `udaserver.cfg` provides a starter policy to which you add
your data paths. Read access does not grant execution permission. Set
`UDA_LANDLOCK=OFF` to disable enforcement.

See [Server sandboxing](/UDA/server_sandboxing) for configuration, examples,
limitations, Linux testing from macOS, and administrator-managed alternatives such
as systemd, AppArmor/SELinux, containers and Seatbelt.

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
