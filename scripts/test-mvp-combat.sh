#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
CXX="${CXX:-g++}"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/dywx-mvp-combat-tests.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT
FLAGS=(-std=c++20 -O2 -Wall -Wextra -Werror -ISource/DywxUI/Public)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  FLAGS=(-std=c++20 -O0 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -ISource/DywxUI/Public)
  # The dot execution service traces child processes; LSan cannot run under ptrace.
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
SOURCES=(Source/DywxUI/Private/MVP/Simulation.cpp Source/DywxUI/Private/MVP/Progression.cpp Source/DywxUI/Private/MVP/ContentCatalog.cpp)
for TEST in mvp_simulation_tests mvp_proc_tests; do
  "$CXX" "${FLAGS[@]}" "${SOURCES[@]}" "tests/$TEST.cpp" -o "$BUILD_DIR/$TEST"
  "$BUILD_DIR/$TEST"
done
