#!/bin/bash

if [ $# -lt 1 ]; then
  echo "Usage: $0 <compiler> [specific_test]"
  echo "  compiler: gcc, clang, or tcc"
  echo "  specific_test: Optional - Run only this test directory"
  exit 1
fi

COMPILER=$1
SPECIFIC_TEST=$2

if ! command -v $COMPILER &> /dev/null; then
  echo "Error: $COMPILER is not installed or not in PATH"
  exit 1
fi

tests=(
  "01-basic-build"
  "02-custom-config"
  "03-generic-flags"
  "04-raylib-build"
  "05-samurai-source-code"
  "06-lua-source-code"
  "07-raylib-source-code"
)

echo "NOTE: Skipping 07-raylib-source-code test due to missing framework linking support"

if [ ! -z "$SPECIFIC_TEST" ]; then
  found=0
  for test in "${tests[@]}"; do
    if [ "$test" == "$SPECIFIC_TEST" ]; then
      found=1
      break
    fi
  done
  
  if [ $found -eq 0 ]; then
    echo "Error: Test '$SPECIFIC_TEST' not found in test suite"
    exit 1
  fi
  
  tests=("$SPECIFIC_TEST")
fi

for test in "${tests[@]}"; do
  
  echo "Running test $test with $COMPILER..."
  
  if [ ! -d "./tests/$test" ]; then
    echo "Error: Test directory ./tests/$test not found"
    continue
  fi
  
  cd "./tests/$test" || continue
  
  rm -rf "build"
  rm -rf "custom-dir"
  rm -f "mate"
  $COMPILER mate.c -o mate
  ./mate
  
  result=$?
  if [ $result -ne 0 ]; then
    echo "❌ Test $test failed with $COMPILER (exit code: $result)"
    cd - > /dev/null
    exit 1
  else
    echo "✅ Test $test passed with $COMPILER"
  fi
  
  cd - > /dev/null
done

echo "All tests completed successfully with $COMPILER! 🎉"
