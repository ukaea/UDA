# CMake cleanup workplan

The current work keeps the 2.x source/API layout and adds an installed-client
contract that can be consumed by wrappers built later. `feature/3.0` is a design
reference; its source refactor is not required here.

## Implemented, pending review

- Centralized install destinations with `UDA_INSTALL_LAYOUT=LEGACY|SYSTEM`.
  Legacy remains the default. System layout honors GNUInstallDirs and has an
  independent systemd unit directory.
- Layout-aware generated scripts, modulefiles, pkg-config and Python build
  inputs. pkg-config retains prefix overrides. Plugin registration supports the
  configured system directory and an explicit `--config-dir` override.
- CMake exports documented as the primary supported cross-platform client
  interface. Unix pkg-config metadata is retained as optional compatibility
  output, generated only for built libraries, with a relative prefix and public
  versus static-link requirements. No build-machine dependency paths are copied
  into the metadata. Thread discovery runs before UDA adds its global compiler
  flags so static pkg-config consumers get any required thread library directly.
- Install component labels, without selecting binary package generators or
  splitting packages yet.
- Installed `UDAConfig.cmake` with the `Client` component and explicit
  `UDA::client_static` / `UDA::client_shared` targets. Internal object libraries
  remain private. Package discovery preserves the caller's module search path.
- Public libxml2 and RPC requirements follow the current public headers.
  OpenSSL, fmt and threads are implementation dependencies, retained on static
  consumers' link lines without propagating their compile settings.
- PIC on the object targets that actually compile the static client. Exported
  static clients define `UDA_STATIC` for Windows header import semantics.
- A standalone IDL configure path using the installed static client. Existing
  GNU/Linux static-libstdc++ handling is retained, with its library search
  changes scoped; Apple Clang uses libc++. SDK discovery handles platform and
  architecture directories and reads the SDK version without starting IDL.
  macOS produces the vendor's `.so` DLM suffix. Linux rejects unresolved symbols
  at link time. The obsolete `CALLIDAM /TEST` path now reports an unsupported
  operation instead of referencing the removed `idamClientTestAPI` core symbol.
- A standalone build for the regular C++ wrapper. The `UDACpp` package exports
  `UDA::cpp_static` and optionally `UDA::cpp_shared`, linked to the corresponding
  installed core targets. Legacy in-tree C++ archives still include the core
  objects. Fat C++ builds remain in-tree.
- An independent installed-consumer project under `test/consumer`, referencing
  real APIs and running a local smoke test without a server connection.
- Separate `test/pkgconfig` consumers exercise legacy flags independently of
  the CMake exports, including core, C++ wrapper and plugin entry points.
- The build CI matrix runs installed core and standalone C++ CMake consumers
  on Linux, macOS and Windows. Linux and macOS also use pkg-config to consume
  both the shared libraries and separately staged static archives.

## Verification completed

- macOS: client static/shared builds, full non-wrapper server/plugin build,
  staged system installation with a multiarch-style library directory.
- macOS: installed static/shared consumer builds and runtime smoke tests,
  repeated after moving the installation and removing the old prefix.
- Ubuntu 24.04 arm64 / GCC 13: installed static/shared consumers and relocated
  static consumer build/run, including rediscovery of TIRPC. System configure
  selects the native multiarch library directory.
- Independent probes: pkg-config prefix overrides, independent systemd path
  with `lib64`, plugin install/list/uninstall using configured system paths,
  explicit plugin-directory overrides, and optional package discovery when a
  dependency is missing.
- Real IDL 9.3 macOS arm64 SDK: both wrapper sources compile, link and install
  against an installed static client; the in-tree `uda_idl` target also builds.
  The DLM exports `IDL_Load` and has no unresolved UDA client symbols.
- Real IDL 8.8.1 macOS Intel SDK: both wrapper sources compile to x86_64 objects.
  A complete Intel link still needs a matching Intel UDA/dependency installation.
  The vendor header emits strlcpy/strlcat redefinition warnings with the modern
  macOS SDK. These checks do not start the interpreter or require loading a DLM.
- Real IDL 9.3 Linux x86_64 SDK: standalone compile/link/install and the in-tree
  `uda_idl` build pass on AlmaLinux 8 with GCC 14.2.1 and CMake 3.26.5 under x86
  emulation. The standalone DLM exports `IDL_Load`, has no unresolved UDA symbols,
  and passes the strict Linux link check. The image's bundled CMake crashed in
  QEMU before configuration; the distribution CMake completed the checks.
  No IDL interpreter was started.
- macOS and Ubuntu arm64: pkg-config core shared/static consumers and static
  C++ wrapper consumers build and run. Relocation to a prefix containing spaces
  passes with pkgconf 2.5.1 (macOS) and pkg-config 1.8.1 (Ubuntu). Nonstandard
  Boost headers are supplied through the consumer's normal toolchain flags.
- Ubuntu: reconfiguring a C++ build with wrappers disabled and installing into
  a clean prefix does not install stale C++ metadata. libxml2's static dependency
  closure requires its development dependencies (liblzma-dev in the test image).
- AlmaLinux 8.10 x86_64: `uda-client.pc` now lists `-lpthread` in
  `Libs.private`. The independent static pkg-config and installed CMake
  consumers both link and pass their smoke checks when launched directly.
  The old metadata also linked on this image because TIRPC and OpenSSL
  contributed thread flags independently.
- macOS: fat-client and plugin pkg-config smoke tests pass in a staged system
  install. Disabling pkg-config output still leaves a working CMake package.
- macOS arm64: standalone static and shared C++ wrapper builds install and
  pass independent downstream CMake smoke tests. The shared build requires the
  installed shared core, and the static archive contains wrapper objects only.
  The regular and fat in-tree C++ targets also build as static and shared libraries.
- macOS arm64 and AlmaLinux 8.10 x86_64: the CI consumer fixtures pass for the
  static and shared core and C++ CMake targets, and for static and shared
  `uda-client` and `uda-cpp` pkg-config consumers.

## Next implementation and validation

1. Review the change series as install policy, client package contract, and IDL
   consumption changes. They are currently uncommitted in the working tree.
2. Validate Windows compilation/linking through the core and C++ CMake
   exports, including static headers and DLL import/export. Legacy pkg-config
   metadata is limited to Unix toolchains.
3. Complete SDK compilation coverage on Windows. When a licensed IDL
   interpreter/VM is available, load the DLM and exercise normal data/structured
   requests and error paths, including the unsupported legacy TEST keyword.
   Check against the actual deployed IDL version and its preloaded C++ runtime;
   a modern SDK compile does not establish compatibility with the GCC 4.8.5 host.
4. Port the RPM/DEB work separately. Verify staged package contents and service
   configuration, config-file preservation, install component dependencies,
   runtime/log directory ownership, and clean package installation/removal.

## Deferred

- Private C++ runtime/symbol isolation against the GCC 4.8.5 IDL host and the
  static C++ dependency closure. The current static-libstdc++ workaround is not
  proof of isolation. The Linux test DLM has no direct `libstdc++.so` requirement,
  but its shared dependency closure still needs a runtime audit.
- IDL's Linux 9.3 SDK bundles `libtirpc.so.3`; CMake warns that the SDK library
  directory can hide system TIRPC in the build's runtime search path. Installation
  removes that build path, but the actual interpreter's loaded RPC libraries
  still need checking alongside its C++ runtime. A successful SDK link does not
  resolve this runtime compatibility question.
- Creation of service accounts and ownership of log/runtime directories.
- Binary package splitting, dedicated packaging CI, and the 3.0
  component/source reorganization.
- Broader cleanup of legacy global compiler definitions/options and the
  client2/fat-client export contracts.
- Memcached was an incomplete experiment and is not on the validation critical
  path. Disabled builds no longer discover or advertise it through the client
  metadata; removal/deprecation can be handled separately.
