#!/usr/bin/env bash
#
# Runs the test suite against a single compiler.
#   ./run-tests.sh <compiler> [test]
#
# Tests run in parallel (MATE_TEST_JOBS, defaults to nproc) and the run aborts
# as soon as one of them fails, killing whatever is still in flight. Output of
# a test is only shown when it fails, or when a single test was asked for.
#
#   MATE_TEST_JOBS     parallel tests, default: nproc
#   MATE_TEST_TIMEOUT  seconds a single ./mate run may take, default: 30
#
# btw why is bash so cursed?
set -u -m

TESTS=(
  "01-basic-build"
  "02-custom-config"
  "03-generic-flags"
  "04-raylib-build"
  "05-samurai-source-code"
  "06-lua-source-code"
  "07-raylib-source-code"
  "08-custom-compiler"
  "09-shared-lib"
  "10-cross-compile"
  "11-output-paths"
)

declare -A SKIP=(
  [windows]=" 05-samurai-source-code 06-lua-source-code 10-cross-compile 11-output-paths "
  [tcc]=" 07-raylib-source-code "
  [filc]=" 07-raylib-source-code 10-cross-compile 11-output-paths "
)

if [ $# -lt 1 ]; then
  echo "Usage: $0 <compiler> [test]"
  exit 1
fi

COMPILER="$1"
SPECIFIC_TEST="${2:-}"
JOBS="${MATE_TEST_JOBS:-$(nproc 2>/dev/null || echo 4)}"
TEST_TIMEOUT="${MATE_TEST_TIMEOUT:-30}"
CNAME="$(basename "$COMPILER")"

case "$CNAME" in cl|cl.exe) IS_MSVC=true ;; *) IS_MSVC=false ;; esac
case "$OSTYPE" in msys*|cygwin*|win32*) EXE=".exe"; IS_WINDOWS=true ;; *) EXE=""; IS_WINDOWS=false ;; esac

command -v "$COMPILER" &>/dev/null || { echo "Error: $COMPILER is not installed or not in PATH"; exit 1; }

skip_reason() {
  local t=" $1 "
  "$IS_WINDOWS" && [[ "${SKIP[windows]}" == *"$t"* ]] && { echo "not supported on Windows"; return; }
  [[ "${SKIP[$CNAME]:-}" == *"$t"* ]] && { echo "excluded for $CNAME"; return; }
}

[ -n "$SPECIFIC_TEST" ] && TESTS=("$SPECIFIC_TEST")

RUN_LIST=()
for test in "${TESTS[@]}"; do
  reason="$(skip_reason "$test")"
  if [ -n "$reason" ]; then
    echo "Skipping $test ($reason)"
  else
    RUN_LIST+=("$test")
  fi
done
[ "${#RUN_LIST[@]}" -gt 0 ] || { echo "Nothing to run with $COMPILER"; exit 0; }

ROOT_DIR="$(pwd)"
WORK_DIR="$(mktemp -d)"

mkfifo "$WORK_DIR/results"
exec 3<>"$WORK_DIR/results"

declare -A JOB_PIDS=()

kill_running() {
  local pid
  for pid in "${JOB_PIDS[@]}"; do
    kill -TERM -- "-$pid" 2>/dev/null || kill -TERM "$pid" 2>/dev/null
  done
  wait 2>/dev/null
}

cleanup() {
  kill_running
  exec 3>&- 3<&-
  rm -rf "$WORK_DIR"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

compile() {
  if "$IS_MSVC"; then
    "$COMPILER" -w -nologo mate.c -Fe:"mate${EXE}" 2>&1 | tr -d '\r' | grep -vx "mate.c"
    return "${PIPESTATUS[0]}"
  fi
  "$COMPILER" -w mate.c -o "mate${EXE}"
}

run_one() {
  local test="$1" code
  cd "$ROOT_DIR/tests/$test" || { echo "no such directory tests/$test"; return 1; }
  rm -rf build custom-dir "mate${EXE}" mate.obj mate.pdb mate.ilk

  compile || { echo "compiling mate.c failed"; return 1; }

  timeout "$TEST_TIMEOUT" "./mate${EXE}"
  code=$?
  case "$code" in
    0)   ;;
    124) echo "timed out after ${TEST_TIMEOUT}s"; return 1 ;;
    *)   echo "./mate${EXE} exited with $code"; return 1 ;;
  esac

  compile || { echo "recompiling mate.c failed"; return 1; }
}

start_one() {
  local test="$1"
  {
    if run_one "$test" >"$WORK_DIR/$test.log" 2>&1; then
      echo "$test PASSED" >&3
    else
      echo "$test FAILED" >&3
    fi
  } &
  JOB_PIDS["$test"]=$!
}

started=0
running=0
status=0

while [ "$started" -lt "${#RUN_LIST[@]}" ] || [ "$running" -gt 0 ]; do
  while [ "$started" -lt "${#RUN_LIST[@]}" ] && [ "$running" -lt "$JOBS" ]; do
    start_one "${RUN_LIST[$started]}"
    started=$((started + 1))
    running=$((running + 1))
  done

  read -r test result <&3
  running=$((running - 1))
  unset 'JOB_PIDS[$test]'

  echo "$test $result"
  if [ "$result" = "FAILED" ]; then
    status=1
    break
  fi
  [ -n "$SPECIFIC_TEST" ] && cat "$WORK_DIR/$test.log"
done

if [ "$status" -ne 0 ]; then
  echo ""
  cat "$WORK_DIR/$test.log"
  echo ""
  echo "$test FAILED with $COMPILER, aborting"
else
  echo ""
  echo "All tests PASSED with $COMPILER"
fi
exit "$status"
