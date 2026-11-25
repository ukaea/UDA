# Installed client check

This standalone project finds an already installed UDA package. It calls a local
version accessor without contacting a server, and references the request API so
the static link must resolve the client implementation and its dependencies.

```sh
cmake -S test/consumer -B consumer-build -DCMAKE_PREFIX_PATH=/path/to/uda
cmake --build consumer-build
ctest --test-dir consumer-build --output-on-failure
```

For a shared-library installation, repeat in a separate build directory with
`-DUDA_CONSUMER_TARGET=UDA::client_shared`. Both cases require the `Client`
component and verify that package discovery preserves `CMAKE_MODULE_PATH`.

To check a standalone C++ wrapper, set `CMAKE_PREFIX_PATH` to the wrapper and
core install prefixes, separated by a semicolon, and select
`-DUDA_CONSUMER_TARGET=UDA::cpp_static` or `UDA::cpp_shared`. The fixture then
finds `UDACpp` and links a program that uses the wrapper and core APIs.

To check relocation, move the installation to a new prefix and repeat the check
in a fresh consumer build directory using the new prefix. The original prefix
must no longer exist, otherwise a stale absolute path could hide a failure.

These checks can be run by a developer or an external package builder; they are
not tied to a particular container, CI provider, or install workflow.
