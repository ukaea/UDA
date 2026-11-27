#!/usr/bin/env bash
# Builds the UDA dependencies from source for the macOS wheels, at the given deployment target.
# Building from source (rather than Homebrew bottles) keeps the wheel compatible with older macOS
# releases, because Homebrew bottles are built for the newest macOS version of the runner.
#
# Usage: build-macos-deps.sh PREFIX DEPLOYMENT_TARGET
#   PREFIX             where the dependencies are installed; the build is skipped if it is already there
#   DEPLOYMENT_TARGET  minimum macOS version, e.g. 13.0
#
# The versions below are pinned. OpenSSL is kept on the 3.5 LTS line because 4.x deprecates
# X509_cmp_time, which UDA uses.
set -euo pipefail

if [[ $# -ne 2 ]]; then
  sed -n '2,9p' "$0" >&2
  exit 2
fi

prefix=$1
target=$2
marker="$prefix/.complete-$target"

if [[ -f "$marker" ]]; then
  echo "macOS $target dependencies already built in $prefix"
  exit 0
fi

openssl_version=3.5.9
libxml2_series=2.14
libxml2_version=2.14.6
fmt_version=12.2.1
spdlog_version=1.17.0
capnp_version=1.5.0
boost_version=1.92.0

export MACOSX_DEPLOYMENT_TARGET=$target
jobs=$(sysctl -n hw.ncpu)
arch=$(uname -m)
src=$(mktemp -d)
trap 'rm -rf "$src"' EXIT
mkdir -p "$prefix"
prefix=$(cd "$prefix" && pwd)

fetch() {
  curl --fail --location --retry 3 --silent --show-error -o "$src/$(basename "$1")" "$1"
}

# CMake dependencies share one configure, build and install recipe.
cmake_build() {
  local name=$1 dir=$2
  shift 2
  cmake -S "$dir" -B "$src/build-$name" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$target" -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_INSTALL_RPATH="$prefix/lib" -DBUILD_TESTING=OFF "$@"
  cmake --build "$src/build-$name" -j "$jobs"
  cmake --install "$src/build-$name"
}

echo "== OpenSSL $openssl_version"
fetch "https://github.com/openssl/openssl/releases/download/openssl-$openssl_version/openssl-$openssl_version.tar.gz"
tar -xzf "$src/openssl-$openssl_version.tar.gz" -C "$src"
(
  cd "$src/openssl-$openssl_version"
  openssl_target=darwin64-x86_64-cc
  [[ "$arch" == arm64 ]] && openssl_target=darwin64-arm64-cc
  ./Configure "$openssl_target" --prefix="$prefix" --openssldir="$prefix/ssl" \
    no-tests shared
  make -j "$jobs"
  make install_sw
)

echo "== libxml2 $libxml2_version"
fetch "https://download.gnome.org/sources/libxml2/$libxml2_series/libxml2-$libxml2_version.tar.xz"
tar -xJf "$src/libxml2-$libxml2_version.tar.xz" -C "$src"
cmake_build libxml2 "$src/libxml2-$libxml2_version" \
  -DLIBXML2_WITH_PYTHON=OFF -DLIBXML2_WITH_LZMA=OFF -DLIBXML2_WITH_ZLIB=ON \
  -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_PROGRAMS=OFF -DBUILD_SHARED_LIBS=ON

echo "== fmt $fmt_version"
fetch "https://github.com/fmtlib/fmt/archive/refs/tags/$fmt_version.tar.gz"
tar -xzf "$src/$fmt_version.tar.gz" -C "$src"
cmake_build fmt "$src/fmt-$fmt_version" -DFMT_TEST=OFF -DFMT_DOC=OFF -DBUILD_SHARED_LIBS=ON

echo "== spdlog $spdlog_version"
fetch "https://github.com/gabime/spdlog/archive/refs/tags/v$spdlog_version.tar.gz"
tar -xzf "$src/v$spdlog_version.tar.gz" -C "$src"
cmake_build spdlog "$src/spdlog-$spdlog_version" \
  -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_BUILD_TESTS=OFF -DSPDLOG_FMT_EXTERNAL=ON \
  -DSPDLOG_INSTALL=ON -DBUILD_SHARED_LIBS=ON

echo "== Cap'n Proto $capnp_version"
fetch "https://github.com/capnproto/capnproto/archive/refs/tags/v$capnp_version.tar.gz"
tar -xzf "$src/v$capnp_version.tar.gz" -C "$src"
cmake_build capnp "$src/capnproto-$capnp_version/c++" -DBUILD_SHARED_LIBS=ON

echo "== Boost $boost_version (headers, program_options)"
boost_name=boost_${boost_version//./_}
fetch "https://archives.boost.io/release/$boost_version/source/$boost_name.tar.gz"
tar -xzf "$src/$boost_name.tar.gz" -C "$src"
(
  cd "$src/$boost_name"
  ./bootstrap.sh --prefix="$prefix" --with-libraries=program_options
  ./b2 install -j "$jobs" --prefix="$prefix" --with-program_options \
    variant=release link=shared,static threading=multi \
    cxxflags="-mmacosx-version-min=$target" linkflags="-mmacosx-version-min=$target"
  # Boost.Format, Algorithm and MultiArray are header-only and are not part of 'b2 install'.
  ./b2 headers
  cp -R boost "$prefix/include/"
)

touch "$marker"
echo "macOS $target dependencies installed to $prefix"
