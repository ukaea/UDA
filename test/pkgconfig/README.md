# Legacy pkg-config check

Point `PKG_CONFIG_PATH` at the installed UDA metadata and configure this
standalone project. It builds one program against the client (`uda-client`) and
one against the C++ wrapper (`uda-cpp`), taking every flag from pkg-config rather
than from the UDA CMake package.

```sh
export PKG_CONFIG_PATH=/path/to/uda/lib/pkgconfig
cmake -S test/pkgconfig -B pc-consumer
cmake --build pc-consumer
ctest --test-dir pc-consumer --output-on-failure
```

For an installation built with `BUILD_SHARED_LIBS=OFF`, use a fresh consumer
directory and `-DUDA_PC_STATIC=ON`. This requests `pkg-config --static`, which
expands the private dependencies. Installing only the development component
(`cmake --install build --component development`) to a separate prefix leaves
just the static archives, so the linker must choose them.

C++ wrapper consumers need Boost headers in the compiler's search path. For a
nonstandard Boost installation, supply normal toolchain include flags, for
example `-DCMAKE_CXX_FLAGS=-I/path/to/boost/include`. Boost has no universally
available pkg-config module, so UDA does not embed its build-machine path.

For relocated or space-containing prefixes, use a fresh consumer directory
and point `PKG_CONFIG_PATH` at the new location. To run shared consumers on Unix,
the installed libraries must be on the runtime search path (for example using
`LD_LIBRARY_PATH` on Linux or `DYLD_LIBRARY_PATH` on macOS).
