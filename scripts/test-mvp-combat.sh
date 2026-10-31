#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
CXX="${CXX:-g++}"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/dywx-mvp-combat-tests.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT
FLAGS=(-std=c++20 -O2 -Wall -Wextra -Werror -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  FLAGS=(-std=c++20 -O0 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public)
  # The dot execution service traces child processes; LSan cannot run under ptrace.
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
SOURCES=(Source/DywxCore/Private/MVP/Simulation.cpp Source/DywxCore/Private/MVP/Progression.cpp Source/DywxCore/Private/MVP/ContentCatalog.cpp Source/DywxCore/Private/MVP/{Campaign,Economy,Arena,SaveCodec}.cpp)
for TEST in mvp_simulation_tests mvp_proc_tests; do
  "$CXX" "${FLAGS[@]}" "${SOURCES[@]}" "tests/$TEST.cpp" -o "$BUILD_DIR/$TEST"
  "$BUILD_DIR/$TEST"
done
