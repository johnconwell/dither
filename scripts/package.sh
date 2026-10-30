#!/usr/bin/env bash
# Packages an already-built bin/dither into a release archive in dist/.
# Usage: scripts/package.sh VERSION PLATFORM
#   PLATFORM is <os>-<arch>, e.g. linux-x86_64, macos-arm64, windows-x86_64.
#   Windows platforms produce a .zip, all others a .tar.gz.
# The archive holds one folder, dither-VERSION-PLATFORM/, containing the
# executable, README.md, LICENSE.txt and the example csv files in data/.
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 VERSION PLATFORM" >&2
    exit 2
fi

version=$1
platform=$2
name="dither-${version}-${platform}"

exe=""
if [[ $platform == windows-* ]]; then
    exe=".exe"
fi

cd "$(dirname "$0")/.."

if [[ ! -f "bin/dither${exe}" ]]; then
    echo "bin/dither${exe} not found; run make first" >&2
    exit 1
fi

rm -rf "dist/${name}" "dist/${name}.tar.gz" "dist/${name}.zip"
mkdir -p "dist/${name}"

cp "bin/dither${exe}" "dist/${name}/"
strip "dist/${name}/dither${exe}"
cp README.md LICENSE.txt "dist/${name}/"
cp -R data "dist/${name}/"
find "dist/${name}/data" -name .gitkeep -delete

cd dist
if [[ -n $exe ]]; then
    zip -qr "${name}.zip" "${name}"
    echo "dist/${name}.zip"
else
    tar -czf "${name}.tar.gz" "${name}"
    echo "dist/${name}.tar.gz"
fi
