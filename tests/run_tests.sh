#!/usr/bin/env bash
set -u

BIN="bin/neplang"
pass=0
fail=0

for DIR in tests/*/; do
DIR="${DIR%/}"
echo "== $(basename "$DIR")"

for src in "$DIR"/*.error.np; do
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

for src in "$DIR"/*.np; do
    [ -e "$src" ] || continue
    case "$src" in
        *.error.np) continue ;;
    esac
    name=$(basename "$src" .np)
    expected_file="$DIR/$name.expected"
    [ -e "$expected_file" ] || continue

    actual=$("$BIN" "$src" 2>/dev/null)
    expected=$(cat "$expected_file")

    if [ "$actual" == "$expected" ]; then
        echo "PASS  $name.np"
        pass=$((pass + 1))
    else
        echo "FAIL  $name.np  (expected '$expected', got '$actual')"
        fail=$((fail + 1))
    fi
done

done

echo "== cli"
if "$BIN" tests/calculator/basic.expected >/dev/null 2>&1; then
    echo "FAIL  non-.np file was accepted"
    fail=$((fail + 1))
else
    echo "PASS  non-.np file rejected"
    pass=$((pass + 1))
fi

echo ""
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
