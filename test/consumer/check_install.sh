#!/bin/sh
#
# Compile and run minimal C/C++ consumers against an INSTALLED UDA client,
# using only the flags published by pkg-config.
#
# The system pkg-config search path is deliberately replaced by the install's
# own pkgconfig directory, so any dependency the installed .pc files fail to
# declare - an include directory, a link library, a private dependency needed
# for static links, or a Requires: on a package that isn't there - shows up
# here as a pkg-config, compile or link failure instead of breaking a
# downstream application.
#
# usage: check_install.sh <install-prefix>
#
# exit status: 0 = passed, 77 = skipped (no client installed at prefix), 1 = failed
#
# Keep this POSIX sh: it runs on the analysis cluster as well as in CI.

set -eu

SRCDIR=$(cd "$(dirname "$0")" && pwd)

PREFIX="${1:-}"
if [ -z "${PREFIX}" ]; then
    echo "usage: $0 <install-prefix>" >&2
    exit 1
fi
if [ ! -d "${PREFIX}" ]; then
    echo "SKIP: no such install prefix: ${PREFIX}"
    exit 77
fi

PKGDIR="${PREFIX}/lib/pkgconfig"
if [ ! -f "${PKGDIR}/uda-client.pc" ]; then
    echo "SKIP: ${PKGDIR}/uda-client.pc not found - install the client before running this test"
    exit 77
fi

if ! command -v pkg-config > /dev/null 2>&1; then
    echo "SKIP: pkg-config not available"
    exit 77
fi

CC_BIN="${CC:-cc}"
CXX_BIN="${CXX:-c++}"
CMAKE_BIN="${CMAKE:-cmake}"

# Only the installed .pc files are visible: no system fmt.pc, openssl.pc, etc.
PKG_CONFIG_LIBDIR="${PKGDIR}"
export PKG_CONFIG_LIBDIR
unset PKG_CONFIG_PATH || true

TMP=$(mktemp -d "${TMPDIR:-/tmp}/uda-consumer.XXXXXX")
trap 'rm -rf "${TMP}"' EXIT HUP INT TERM

failures=0

fail()
{
    echo "FAIL: $*"
    failures=$((failures + 1))
}

pass()
{
    echo "PASS: $*"
}

echo "== checking UDA install at ${PREFIX}"

#-----------------------------------------------------------------------------
# 1. every installed .pc file must be usable, and name libraries that exist
#-----------------------------------------------------------------------------
for pc in "${PKGDIR}"/*.pc; do
    [ -f "${pc}" ] || continue
    module=$(basename "${pc}" .pc)

    if pkg-config --exists --print-errors "${module}" > "${TMP}/pc.log" 2>&1; then
        pass "${module}.pc resolves"
    else
        sed 's/^/      /' "${TMP}/pc.log"
        fail "${module}.pc does not resolve (undeclared or missing dependency)"
        continue
    fi

    for lib in $(sed -n 's/^Libs:/ /p' "${pc}" | tr ' ' '\n' | sed -n 's/^-l\(uda[A-Za-z0-9_]*\)$/\1/p'); do
        found=0
        for ext in a so dylib; do
            if [ -f "${PREFIX}/lib/lib${lib}.${ext}" ]; then
                found=1
            fi
        done
        if [ "${found}" -eq 0 ]; then
            fail "${module}.pc links -l${lib} but no lib${lib}.{a,so,dylib} is installed"
        fi
    done
done

#-----------------------------------------------------------------------------
# 2. pkg-config files belong in lib/pkgconfig, not among the modulefiles
#-----------------------------------------------------------------------------
if [ -d "${PREFIX}/modulefiles" ]; then
    stray=$(find "${PREFIX}/modulefiles" -name '*.pc' | tr '\n' ' ')
    if [ -n "${stray}" ]; then
        fail "pkg-config files installed under modulefiles: ${stray}"
    else
        pass "no stray .pc files under modulefiles"
    fi
fi

#-----------------------------------------------------------------------------
# 3. C consumer, linked shared, using only pkg-config flags
#-----------------------------------------------------------------------------
CFLAGS_LIBS=$(pkg-config --cflags --libs uda-client 2> /dev/null || true)
# shellcheck disable=SC2086
if [ -z "${CFLAGS_LIBS}" ]; then
    fail "pkg-config produced no flags for uda-client - skipping the C build"
elif ${CC_BIN} -o "${TMP}/consumer_c" "${SRCDIR}/consumer_c.c" ${CFLAGS_LIBS} \
        -Wl,-rpath,"${PREFIX}/lib" > "${TMP}/cc.log" 2>&1; then
    if "${TMP}/consumer_c" > "${TMP}/cc.run" 2>&1; then
        pass "C consumer builds and runs (shared)"
        sed 's/^/      /' "${TMP}/cc.run"
    else
        sed 's/^/      /' "${TMP}/cc.run"
        fail "C consumer built but did not run"
    fi
else
    sed 's/^/      /' "${TMP}/cc.log"
    fail "C consumer failed to build against uda-client.pc"
fi

#-----------------------------------------------------------------------------
# 4. C consumer, linked static, using only "pkg-config --static" flags
#    (the archive is named by path: macOS prefers the shared library when both
#    are present, which would silently skip the static link)
#-----------------------------------------------------------------------------
STATIC_LIB="${PREFIX}/lib/libuda_client.a"
if [ -f "${STATIC_LIB}" ]; then
    STATIC_FLAGS=$(pkg-config --static --cflags --libs uda-client 2> /dev/null | sed "s|-luda_client|${STATIC_LIB}|" || true)
    # shellcheck disable=SC2086
    if [ -z "${STATIC_FLAGS}" ]; then
        fail "pkg-config --static produced no flags for uda-client - skipping the static link check"
    elif ${CC_BIN} -o "${TMP}/consumer_c_static" "${SRCDIR}/consumer_c.c" ${STATIC_FLAGS} \
            > "${TMP}/cc_static.log" 2>&1; then
        if "${TMP}/consumer_c_static" > "${TMP}/cc_static.run" 2>&1; then
            pass "C consumer builds and runs (static)"
        else
            sed 's/^/      /' "${TMP}/cc_static.run"
            fail "static C consumer built but did not run"
        fi
    else
        sed 's/^/      /' "${TMP}/cc_static.log"
        fail "static C consumer failed to link (missing Libs.private entries?)"
    fi
else
    echo "note: no libuda_client.a installed - skipping static link check"
fi

#-----------------------------------------------------------------------------
# 5. C++ consumer, if the C++ wrapper was installed
#-----------------------------------------------------------------------------
if [ -f "${PKGDIR}/uda-cpp.pc" ]; then
    CXXFLAGS_LIBS=$(pkg-config --cflags --libs uda-cpp 2> /dev/null || true)
    # shellcheck disable=SC2086
    if [ -z "${CXXFLAGS_LIBS}" ]; then
        fail "pkg-config produced no flags for uda-cpp - skipping the C++ build"
    elif ${CXX_BIN} -o "${TMP}/consumer_cpp" "${SRCDIR}/consumer_cpp.cpp" ${CXXFLAGS_LIBS} \
            -Wl,-rpath,"${PREFIX}/lib" > "${TMP}/cxx.log" 2>&1; then
        if "${TMP}/consumer_cpp" > "${TMP}/cxx.run" 2>&1; then
            pass "C++ consumer builds and runs (shared)"
            sed 's/^/      /' "${TMP}/cxx.run"
        else
            sed 's/^/      /' "${TMP}/cxx.run"
            fail "C++ consumer built but did not run"
        fi
    else
        sed 's/^/      /' "${TMP}/cxx.log"
        fail "C++ consumer failed to build against uda-cpp.pc"
    fi
else
    echo "note: no uda-cpp.pc installed - skipping C++ checks"
fi

#-----------------------------------------------------------------------------
# 6. the same consumers built the way a downstream CMake project would
#-----------------------------------------------------------------------------
if command -v "${CMAKE_BIN}" > /dev/null 2>&1; then
    if "${CMAKE_BIN}" -S "${SRCDIR}" -B "${TMP}/cmake" > "${TMP}/cmake.log" 2>&1 \
            && "${CMAKE_BIN}" --build "${TMP}/cmake" >> "${TMP}/cmake.log" 2>&1; then
        if ( cd "${TMP}/cmake" && ctest --output-on-failure ) >> "${TMP}/cmake.log" 2>&1; then
            pass "downstream CMake project builds and its tests pass"
        else
            tail -30 "${TMP}/cmake.log" | sed 's/^/      /'
            fail "downstream CMake project built but its tests failed"
        fi
    else
        tail -30 "${TMP}/cmake.log" | sed 's/^/      /'
        fail "downstream CMake project failed to configure or build"
    fi
else
    echo "note: cmake not available - skipping CMake consumer check"
fi

echo
if [ "${failures}" -eq 0 ]; then
    echo "all consumer checks passed"
    exit 0
fi

echo "${failures} consumer check(s) failed"
exit 1
