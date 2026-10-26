#!/bin/sh
# Fills in the Homebrew, Scoop and winget templates for one release.
#
#   packaging/render.sh VERSION SHA256SUMS OUT_DIR
#
# VERSION is without the leading "v" (e.g. 0.5.0). SHA256SUMS is the checksum file
# attached to the GitHub Release. Rendered files are written to OUT_DIR:
#   OUT_DIR/homebrew/neplang.rb
#   OUT_DIR/scoop/neplang.json
#   OUT_DIR/winget/<winget manifests>
set -eu

[ $# -eq 3 ] || { echo "usage: $0 VERSION SHA256SUMS OUT_DIR" >&2; exit 1; }
version=$1
sums=$2
out=$3
here=$(cd "$(dirname "$0")" && pwd)

sha() {
    value=$(awk -v f="$1" '$2 == f || $2 == "*" f { print $1 }' "$sums")
    [ -n "$value" ] || { echo "render.sh: no checksum for $1 in $sums" >&2; exit 1; }
    echo "$value"
}

sha_macos=$(sha neplang-macos-universal)
sha_linux_x86_64=$(sha neplang-linux-x86_64)
sha_linux_arm64=$(sha neplang-linux-arm64)
sha_windows=$(sha neplang-windows-x86_64.exe)
sha_windows_upper=$(echo "$sha_windows" | tr 'a-f' 'A-F')

render() {
    mkdir -p "$(dirname "$2")"
    sed -e "s/@VERSION@/$version/g" \
        -e "s/@SHA_MACOS@/$sha_macos/g" \
        -e "s/@SHA_LINUX_X86_64@/$sha_linux_x86_64/g" \
        -e "s/@SHA_LINUX_ARM64@/$sha_linux_arm64/g" \
        -e "s/@SHA_WINDOWS@/$sha_windows/g" \
        -e "s/@SHA_WINDOWS_UPPER@/$sha_windows_upper/g" \
        "$1" > "$2"
}

render "$here/homebrew/neplang.rb.in" "$out/homebrew/neplang.rb"
render "$here/scoop/neplang.json.in" "$out/scoop/neplang.json"
for f in "$here"/winget/*.yaml.in; do
    name=$(basename "$f" .in)
    render "$f" "$out/winget/$name"
done

echo "Rendered packaging files for $version into $out"
