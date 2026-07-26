#!/usr/bin/env bash

# Runs the test suite against every compiler, stops at the first one that fails.
#   ./run-all-tests.sh [compiler...]
set -u

COMPILERS=("$@")
[ "${#COMPILERS[@]}" -gt 0 ] || COMPILERS=(gcc clang tcc)

for cc in "${COMPILERS[@]}"; do
  if ! command -v "$cc" &>/dev/null; then
    echo "=== $cc (not installed, skipping) ==="
    continue
  fi

  echo "=== $cc ==="
  ./run-tests.sh "$cc" || exit 1
  echo ""
done
