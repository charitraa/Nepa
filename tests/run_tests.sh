#!/usr/bin/env bash
set -u

BIN="bin/nepa"
pass=0
fail=0

for DIR in tests/*/; do
DIR="${DIR%/}"
echo "== $(basename "$DIR")"

for src in "$DIR"/*.error.nepa; do
    [ -e "$src" ] || continue
    name=$(basename "$src")
    "$BIN" "$src" >/dev/null 2>/dev/null
    code=$?
    if [ "$code" -ne 0 ]; then
        echo "PASS  $name (correctly failed)"
        pass=$((pass + 1))
    else
        echo "FAIL  $name (expected failure, got exit code 0)"
        fail=$((fail + 1))
    fi
done

for src in "$DIR"/*.nepa; do
    [ -e "$src" ] || continue
    case "$src" in
        *.error.nepa) continue ;;
    esac
    name=$(basename "$src" .nepa)
    expected_file="$DIR/$name.expected"
    [ -e "$expected_file" ] || continue

    actual=$("$BIN" "$src" 2>/dev/null)
    expected=$(cat "$expected_file")

    if [ "$actual" == "$expected" ]; then
        echo "PASS  $name.nepa"
        pass=$((pass + 1))
    else
        echo "FAIL  $name.nepa  (expected '$expected', got '$actual')"
        fail=$((fail + 1))
    fi
done

done

echo ""
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
