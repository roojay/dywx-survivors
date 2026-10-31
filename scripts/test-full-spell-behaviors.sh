#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD=$(mktemp -d); trap 'rm -rf "$BUILD"' EXIT
FLAGS=(-std=c++20 -O1 -Wall -Wextra -Werror -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  FLAGS=(-std=c++20 -O0 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public)
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
${CXX:-g++} "${FLAGS[@]}" Source/DywxCore/Private/MVP/{Simulation,Progression,ContentCatalog,Campaign,Economy,Arena,SaveCodec}.cpp tests/full_spell_behavior_tests.cpp -o "$BUILD/test"
"$BUILD/test"
