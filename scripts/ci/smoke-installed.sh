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

run_windows_executables() {
  local native_dir
  native_dir=$(cygpath -w "$1") || return 1
  UDA_SMOKE_CONSUMER_DIR="$native_dir" MSYS2_ARG_CONV_EXCL='*' \
    powershell.exe -NoLogo -NoProfile -NonInteractive -Command '
      $ErrorActionPreference = "Stop"
      Add-Type -Namespace UdaSmoke -Name NativeMethods -MemberDefinition @"
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        public static extern uint SetErrorMode(uint mode);
"@
      # Children inherit this mode and can exit without dismissing a Windows error dialog.
      # SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX
      [void][UdaSmoke.NativeMethods]::SetErrorMode(3)
      $failed = $false
      foreach ($exe in (Get-ChildItem -LiteralPath $env:UDA_SMOKE_CONSUMER_DIR -Filter "*.exe" -File)) {
        $process = New-Object System.Diagnostics.Process
        try {
          $process.StartInfo.FileName = $exe.FullName
          $process.StartInfo.WorkingDirectory = $exe.DirectoryName
          $process.StartInfo.UseShellExecute = $false
          [void]$process.Start()
          if ($process.WaitForExit(30000)) {
            $status = "0x{0:X8}" -f $process.ExitCode
            $detail = switch ($status) {
              "0xC0000135" { " (DLL not found)" }
              "0xC0000139" { " (DLL entry point not found)" }
            }
            Write-Host "$($exe.Name) exited with Windows status $status$detail"
            if ($process.ExitCode -ne 0) { $failed = $true }
          } else {
            Write-Host "error: $($exe.Name) did not exit within 30 s"
            $failed = $true
            $process.Kill()
          }
        } catch {
          Write-Host "error: $($exe.Name): $($_.Exception.Message)"
          $failed = $true
        } finally {
          $process.Dispose()
        }
      }
      exit ([int]$failed)
    '
}

# Inspect regular PE imports before launching the executables. This search approximates the
# Windows loader, so use the native launch result to decide whether the smoke test passed.
check_windows_executables() {
  local dir=$1 windows_dir objdump cxx mingw_bin file imports dll key path
  local -a search_path queue
  local -A resolved=()
  queue=("$dir"/*.exe)
  if [[ ! -f "${queue[0]}" ]]; then
    echo "error: no consumer executables found in $dir" >&2
    return 1
  fi
  windows_dir=$(cygpath -u "${SYSTEMROOT:-C:/Windows}") || return 1
  IFS=: read -r -a search_path <<< "$PATH"
  cxx=$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "$dir/CMakeCache.txt")
  cxx=$(cygpath -u "$cxx") || return 1
  mingw_bin=$(dirname "$cxx")
  objdump="$mingw_bin/objdump.exe"

  find_dll() {
    local d hit
    for d in "$dir" "$windows_dir/System32" "$windows_dir/System" "$windows_dir" "${search_path[@]}"; do
      [[ -d "$d" ]] || continue
      hit=$(find "$d" -maxdepth 1 -type f -iname "$1" -print -quit 2>/dev/null)
      if [[ -n "$hit" ]]; then echo "$hit"; return 0; fi
    done
    return 1
  }

  if [[ -x "$objdump" ]]; then
    echo "Inspecting consumer DLL imports with $objdump"
    while (( ${#queue[@]} )); do
      file=${queue[0]}
      queue=("${queue[@]:1}")
      if ! imports=$("$objdump" -p "$file"); then
        echo "warning: objdump failed for $file" >&2
        continue
      fi
      while IFS= read -r dll; do
        dll=${dll%$'\r'}
        key=${dll,,}
        case "$key" in api-ms-win-*|ext-ms-*) continue ;; esac
        if [[ ! ${resolved[$key]+set} ]]; then
          path=$(find_dll "$dll") || path=
          resolved[$key]=$path
          case "$path" in
            ""|"$windows_dir"/*) ;;
            *) queue+=("$path") ;;
          esac
        fi
        path=${resolved[$key]}
        if [[ -z "$path" ]]; then
          echo "warning: ${file##*/} imports $dll, which the probe could not resolve" >&2
          continue
        fi
        echo "${file##*/} imports $dll -> $path"
        case "$key" in
          libstdc++-6.dll|libgcc_s_*.dll|libwinpthread-1.dll)
            if [[ -f "$mingw_bin/$dll" && ! "$path" -ef "$mingw_bin/$dll" ]]; then
              echo "warning: $dll resolves outside the consumer compiler directory ($mingw_bin)" >&2
            fi
            ;;
        esac
      done < <(sed -n 's/^[[:space:]]*DLL Name: //p' <<< "$imports")
    done
  else
    echo "warning: $objdump is unavailable, skipping DLL import inspection" >&2
  fi

  run_windows_executables "$dir"
}

run_build_and_test() {
  local name=$1
  cmake --build "$name" --config "$config"
  if [[ "$OSTYPE" == msys* || "$OSTYPE" == cygwin* ]]; then
    check_windows_executables "$name" || exit 1
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
