#!/usr/bin/env bash
# Writes SHA256SUMS, dither.rb (Homebrew) and dither.json (Scoop) into DIST_DIR
# from the release archives already in DIST_DIR.
# Usage: scripts/render_packaging.sh VERSION DIST_DIR [OWNER/REPO]
#   OWNER/REPO defaults to $GITHUB_REPOSITORY, then johnconwell/dither.
set -euo pipefail

version=${1:?usage: $0 VERSION DIST_DIR [OWNER/REPO]}
dist=${2:?usage: $0 VERSION DIST_DIR [OWNER/REPO]}
repository=${3:-${GITHUB_REPOSITORY:-johnconwell/dither}}
root="$(cd "$(dirname "$0")/.." && pwd)"

platforms=(linux-x86_64 linux-arm64 macos-arm64 macos-x86_64 windows-x86_64)

archive_name() {
    if [[ $1 == windows-* ]]; then
        echo "dither-${version}-$1.zip"
    else
        echo "dither-${version}-$1.tar.gz"
    fi
}

cd "$dist"

archives=()
for platform in "${platforms[@]}"; do
    archive=$(archive_name "$platform")
    if [[ ! -f $archive ]]; then
        echo "missing $dist/$archive" >&2
        exit 1
    fi
    archives+=("$archive")
done

sha256sum "${archives[@]}" > SHA256SUMS

sed_args=(-e "s|@VERSION@|${version}|g" -e "s|@REPOSITORY@|${repository}|g")
for platform in "${platforms[@]}"; do
    hash=$(sha256sum "$(archive_name "$platform")" | cut -d' ' -f1)
    key=$(echo "$platform" | tr '[:lower:]-' '[:upper:]_')
    sed_args+=(-e "s|@SHA256_${key}@|${hash}|g")
done

sed "${sed_args[@]}" "$root/packaging/homebrew/dither.rb.in" > dither.rb
sed "${sed_args[@]}" "$root/packaging/scoop/dither.json.in" > dither.json

if grep -n "@[A-Z0-9_]*@" dither.rb dither.json; then
    echo "unfilled placeholders remain" >&2
    exit 1
fi

cat SHA256SUMS
