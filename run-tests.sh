#!/usr/bin/env bash
set -u

TESTS=(
  "01-basic-build"
  "02-custom-config"
  "03-generic-flags"
  "04-raylib-build"
  "05-samurai-source-code"
  "06-lua-source-code"
  "07-raylib-source-code"
)

WINDOWS_SKIP=("05-samurai-source-code")

if [ $# -lt 1 ]; then
  echo "Usage: $0 <compiler> [specific_test]"
  echo "  compiler: gcc, clang, or tcc"
  echo "  specific_test: Optional - Run only this test file"
  exit 1
fi

COMPILER="$1"
SPECIFIC_TEST="${2:-}"

case "$OSTYPE" in
  msys*|cygwin*|win32*) EXE=".exe"; IS_WINDOWS=true ;;
  *) EXE=""; IS_WINDOWS=false ;;
esac

if ! command -v "$COMPILER" &> /dev/null; then
  echo "Error: $COMPILER is not installed or not in PATH"
  exit 1
fi

cleanup() {
  rm -rf "build"
  rm -rf "custom-dir"
  rm -f "mate${EXE}"
}

if [ -n "$SPECIFIC_TEST" ]; then
  TESTS=("$SPECIFIC_TEST")
fi

ROOT_DIR="$(pwd)"

for test in "${TESTS[@]}"; do
  if "$IS_WINDOWS"; then
    for skip in "${WINDOWS_SKIP[@]}"; do
      if [ "$test" = "$skip" ]; then
        echo "Skipping $test (not supported on Windows)"
        continue 2
      fi
    done
  fi

  echo ""
  echo "Running $test with $COMPILER..."
  cd "$ROOT_DIR/tests/$test" || { echo "Error: could not cd into tests/$test"; exit 1; }
  cleanup

  if ! "$COMPILER" mate.c -o "mate${EXE}"; then
    echo ""
    echo "Compilation of $test failed"
    exit 1
  fi

  if ! "./mate${EXE}"; then
    echo ""
    echo "$test FAILED with $COMPILER"
    exit 1
  fi

  echo "$test PASSED"
done

echo ""
echo "All tests PASSED with $COMPILER"
