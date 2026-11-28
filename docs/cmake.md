# Building and installing UDA with CMake

## Install layouts

`UDA_INSTALL_LAYOUT` selects where files are installed.

- `LEGACY` (default): everything is relative to `CMAKE_INSTALL_PREFIX`, for example
  `bin/`, `lib/`, `include/uda/`, `etc/` and `modulefiles/uda/`. Existing sites and
  scripts rely on this layout.
- `SYSTEM`: GNU install directories (`CMAKE_INSTALL_BINDIR`, `CMAKE_INSTALL_LIBDIR`, ...).
  Configuration goes to `${sysconfdir}/uda`, logs to `${localstatedir}/log/uda`, and
  the systemd unit directory is set by `UDA_INSTALL_SYSTEMD_UNITDIR`. Packages use this layout.

Every destination is available as a `UDA_INSTALL_*` variable (see `cmake/UDAInstallDirs.cmake`),
and the `UDA_INSTALL_FULL_*` variables hold absolute paths for templates.

## Install components

| Component        | Contents                                                        |
|------------------|-----------------------------------------------------------------|
| `development`   | Headers, static archives, CMake and pkg-config metadata         |
| `client-runtime` | Shared client, C++ wrapper and serialisation libraries           |
| `server`         | Server executables, shared server library and configuration     |
| `plugins`        | Plugin libraries, shared plugin library and their configuration |
| `tools`          | Command-line tools and the module file                          |

The `development` package depends on `client-runtime` when shared libraries are built.

## Consumers

CMake consumers use the installed package:

```cmake
find_package(UDA CONFIG REQUIRED COMPONENTS Client)   # UDA::client_static, UDA::client_shared
find_package(UDACpp CONFIG REQUIRED)                  # UDA::cpp_static, UDA::cpp_shared
```

`UDA::client_shared` and `UDA::cpp_shared` exist only when `client-runtime` is installed.
The C++ wrapper package requires the same UDA version as the core package.

Non-CMake Unix consumers can use the pkg-config files `uda-client`, `uda-cpp`,
`uda-fat-client`, `uda-fat-cpp` and `uda-plugins` (`UDA_INSTALL_PKGCONFIG`, on by default).
They describe the archives or shared libraries that are installed, so a static consumer
must install the development component alone to a separate prefix.

`test/consumer` and `test/pkgconfig` check both kinds of consumer. `scripts/ci/smoke-installed.sh`
runs them against an installed package.

## Library names

On Unix, static archives and shared libraries have the usual names: `libuda_client.a` and
`libuda_client.so` (or `.dylib`).

On Windows a static archive and the import library of a shared library would both be
`*.lib`, so static archives carry a `_static` suffix (`uda_client_static.lib`). The shared
import library is `uda_client.lib` (MSVC) or `libuda_client.a` (MinGW). The DLLs are installed
to `bin/`, next to the executables that load them.

## Export macros

`clientserver/export.h` defines `LIBRARY_API` for the core libraries, and
`wrappers/c++/export.hpp` defines `UDA_CXX_API` for the C++ wrapper. Both are described in
their headers.
