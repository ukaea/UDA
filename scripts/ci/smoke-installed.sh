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

# On Windows a DLL that cannot be found opens a modal dialog, which hangs CI until ctest times out.
# Check each executable's DLL imports first, so the missing DLL is named in the log instead.
check_windows_dlls() {
  local dir=$1 exe dll system_dir missing=0
  command -v objdump >/dev/null 2>&1 || { echo "note: objdump not found, skipping DLL check" >&2; return 0; }
  system_dir=$(cygpath -u "${SYSTEMROOT:-C:/Windows}")
  # True if the DLL is in a PATH directory or the system directory (names are case-insensitive).
  dll_found() {
    local IFS=: d
    for d in $PATH "$system_dir"; do
      [[ -n "$d" && -n "$(find "$d" -maxdepth 1 -iname "$1" -print -quit 2>/dev/null)" ]] && return 0
    done
    return 1
  }
  while IFS= read -r exe; do
    while IFS= read -r dll; do
      case "$dll" in api-ms-win-*|ext-ms-*) continue ;; esac
      if ! dll_found "$dll"; then
        echo "error: $(basename "$exe") needs $dll, which is not on PATH or in $system_dir" >&2
        missing=1
      fi
    done < <(objdump -p "$exe" | sed -n 's/^[[:space:]]*DLL Name: //p')
  done < <(find "$dir" -maxdepth 1 -name '*.exe')
  return $missing
}

run_build_and_test() {
  local name=$1
  cmake --build "$name" --config "$config"
  if [[ "$OSTYPE" == msys* || "$OSTYPE" == cygwin* ]]; then
    check_windows_dlls "$name" || exit 1
  fi
  (cd "$name" && ctest -C "$config" --timeout 120 --output-on-failure)
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
