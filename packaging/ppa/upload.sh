#!/bin/sh
# Builds signed Debian source packages and uploads them to the Launchpad PPA, which
# then compiles NepLang for each Ubuntu release on Launchpad's servers.
#
#   packaging/ppa/upload.sh 0.5.0                 # default Ubuntu releases
#   packaging/ppa/upload.sh 0.5.0 "noble jammy"   # pick releases
#
# Needs: devscripts, debhelper, dput, and a GPG key registered on Launchpad.
# Set PPA to override the target (default: ppa:charitraa/neplang).
set -eu

[ $# -ge 1 ] || { echo "usage: $0 VERSION [\"SERIES ...\"]" >&2; exit 1; }
version=$1
series=${2:-"resolute noble jammy"}
ppa=${PPA:-ppa:charitraa/neplang}

export DEBFULLNAME="${DEBFULLNAME:-Charitra Shrestha}"
export DEBEMAIL="${DEBEMAIL:-stharabi9862187405@gmail.com}"

root=$(git rev-parse --show-toplevel)
work=$(mktemp -d)
echo "Working in $work"

for s in $series; do
    pkgver="${version}~${s}1"
    dir="$work/neplang-$pkgver"
    mkdir -p "$dir"
    git -C "$root" archive HEAD | tar -x -C "$dir"
    (
        cd "$dir"
        dch -b -v "$pkgver" -D "$s" "Build $version for Ubuntu $s."
        debuild -S -sa -d
    )
    dput "$ppa" "$work/neplang_${pkgver}_source.changes"
done

echo "Uploaded. Launchpad builds take a few minutes: https://launchpad.net/~charitraa/+archive/ubuntu/neplang"
