# Installed package check

This standalone project finds an installed UDA package and the standalone C++
wrapper package. It builds one executable for each installed target
(`UDA::client_static`, `UDA::client_shared`, `UDA::cpp_static`, `UDA::cpp_shared`)
and runs each one. The programs call local accessors only and make no server
connection. The client programs also reference the request API so the link must
resolve the client implementation and its dependencies.

Both shared and static targets must be installed. The standalone wrapper must
be installed against the same UDA version.

```sh
cmake -S test/consumer -B consumer-build -DCMAKE_PREFIX_PATH="/path/to/cpp;/path/to/uda"
cmake --build consumer-build
ctest --test-dir consumer-build --output-on-failure
```

To check relocation, move the installation to a new prefix and repeat the check
in a fresh consumer build directory using the new prefix. The original prefix
must no longer exist, otherwise a stale absolute path could hide a failure.

These checks can be run by a developer, by CI, or by a package build. They are
not tied to a particular container or CI provider.
