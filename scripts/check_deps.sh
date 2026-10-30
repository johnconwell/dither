#!/usr/bin/env bash
# Fails if bin/dither links against the compiler's runtime libraries
# (libstdc++, libgcc, winpthread), which users of a prebuilt binary won't have.
# Usage: scripts/check_deps.sh PLATFORM
set -euo pipefail

platform=${1:?usage: $0 PLATFORM}
cd "$(dirname "$0")/.."

case $platform in
    linux-*)
        file bin/dither
        if ! file bin/dither | grep -q "statically linked"; then
            echo "bin/dither is not statically linked" >&2
            exit 1
        fi
        ;;
    macos-*)
        otool -L bin/dither
        if otool -L bin/dither | tail -n +2 | grep -Eq "libstdc\+\+|libgcc|/opt/homebrew|/usr/local"; then
            echo "bin/dither depends on GCC runtime libraries" >&2
            exit 1
        fi
        otool -l bin/dither | grep -A4 LC_BUILD_VERSION || true
        ;;
    windows-*)
        objdump -p bin/dither.exe | grep "DLL Name" || true
        if objdump -p bin/dither.exe | grep "DLL Name" | grep -Eiq "libstdc\+\+|libgcc|libwinpthread"; then
            echo "bin/dither.exe depends on MinGW runtime DLLs" >&2
            exit 1
        fi
        ;;
    *)
        echo "unknown platform: $platform" >&2
        exit 2
        ;;
esac
