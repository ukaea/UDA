# Legacy pkg-config consumer check

Point `PKG_CONFIG_PATH` at the installed UDA metadata and configure this
standalone project. It obtains its flags only from pkg-config, rather than
the UDA CMake exports.

```sh
export PKG_CONFIG_PATH=/path/to/uda/lib/pkgconfig
cmake -S test/pkgconfig -B pc-consumer
cmake --build pc-consumer
ctest --test-dir pc-consumer --output-on-failure
```

For an installation built with `BUILD_SHARED_LIBS=OFF`, use a fresh consumer
directory and `-DUDA_PC_STATIC=ON`. This requests `pkg-config --static`;
that flag expands private dependencies but does not itself force a linker to
choose static archives when shared libraries are also installed.

Select another installed module with `-DUDA_PC_MODULE=uda-cpp`,
`uda-fat-client`, `uda-fat-cpp`, or `uda-plugins`. The C++ cases reference the
C++ wrapper API, and the plugin case calls a plugin file-list function.
All normal smoke-test invocations run without a server connection.

C++ wrapper consumers need Boost headers in the compiler's search path. For a
nonstandard Boost installation, supply normal toolchain include flags, for
example `-DCMAKE_CXX_FLAGS=-I/path/to/boost/include`. Boost has no universally
available pkg-config module, so UDA does not embed its build-machine path.

For relocated or space-containing prefixes, use a fresh consumer directory
and point `PKG_CONFIG_PATH` at the new location. To run shared consumers on Unix,
the installed libraries must be on the runtime search path (for example using
`LD_LIBRARY_PATH` on Linux or `DYLD_LIBRARY_PATH` on macOS).
