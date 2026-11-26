#!/usr/bin/env bash
# Smoke-tests an installed UDA package from outside the source tree, with no server connection.
#
# Usage: smoke-installed.sh CORE_PREFIX CPP_PREFIX BUILD_DIR [CMAKE_ARG...]
#   CORE_PREFIX  where the UDA core package is installed (CMake and pkg-config files)
#   CPP_PREFIX   where the standalone C++ wrapper is built and installed
#   BUILD_DIR    the build tree the package was installed from (used to stage development files)
#   CMAKE_ARG    extra arguments for every CMake configure, e.g. toolchain or Boost flags
#
# Environment:
#   CONFIG     build configuration (default Release)
#   GENERATOR  CMake generator (default Ninja)
#   PKGCONFIG  1 to also check the pkg-config files (default 1; Unix only)
set -euo pipefail

if [[ $# -lt 3 ]]; then
  sed -n '2,11p' "$0" >&2
  exit 2
fi

core_prefix=$1
cpp_prefix=$2
build_dir=$3
shift 3
cmake_args=("$@")

root=$(cd "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
config=${CONFIG:-Release}
generator=${GENERATOR:-Ninja}
pkgconfig=${PKGCONFIG:-1}

# The pkg-config directory is lib/pkgconfig, or lib64/pkgconfig on some SYSTEM layouts.
pkgconfig_dir() {
  find "$1" -name uda-client.pc | sed -n '1{p;q;}' | xargs dirname
}

run_build_and_test() {
  local name=$1
  cmake --build "$name" --config "$config"
  (cd "$name" && ctest -C "$config" --output-on-failure)
}

# Fail early, and say what is installed, if the core package is not where it was expected.
if ! find "$core_prefix" -name UDAConfig.cmake 2>/dev/null | grep -q .; then
  echo "error: no UDAConfig.cmake under $core_prefix; installed tree:" >&2
  find "$core_prefix" -maxdepth 4 >&2 || true
  exit 1
fi

# The standalone wrapper is the C++ consumer's package, so it is built against the installed core.
cmake -G "$generator" -S "$root/source/wrappers/c++" -B "$work/cpp-wrapper" \
  -DCMAKE_BUILD_TYPE="$config" -DBUILD_SHARED_LIBS=ON \
  "-DCMAKE_PREFIX_PATH=$core_prefix" "-DCMAKE_INSTALL_PREFIX=$cpp_prefix" "${cmake_args[@]}"
cmake --build "$work/cpp-wrapper" --config "$config"
cmake --install "$work/cpp-wrapper" --config "$config"

# Each installed target is consumed by its own executable.
cmake -G "$generator" -S "$root/test/consumer" -B "$work/consumer" \
  -DCMAKE_BUILD_TYPE="$config" "-DCMAKE_PREFIX_PATH=$cpp_prefix;$core_prefix" "${cmake_args[@]}"
run_build_and_test "$work/consumer"

if [[ "$pkgconfig" != 1 ]]; then
  exit 0
fi

# Stage only development files, so the linker has to choose the static archives.
static_prefix="$work/static"
cmake --install "$build_dir" --config "$config" --component development --prefix "$static_prefix"
if find "$static_prefix" \( -name 'libuda_*.so*' -o -name 'libuda_*.dylib' \) | grep -q .; then
  echo "error: shared libraries were staged with the development files" >&2
  exit 1
fi

for linkage in shared static; do
  pc_path=$(pkgconfig_dir "$core_prefix")
  pc_static=OFF
  if [[ "$linkage" == static ]]; then
    pc_path="$(pkgconfig_dir "$static_prefix"):$pc_path"
    pc_static=ON
  fi
  PKG_CONFIG_PATH="$pc_path${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" \
    cmake -G "$generator" -S "$root/test/pkgconfig" -B "$work/pkgconfig-$linkage" \
      -DCMAKE_BUILD_TYPE="$config" "-DUDA_PC_STATIC=$pc_static" "${cmake_args[@]}"
  run_build_and_test "$work/pkgconfig-$linkage"
done
