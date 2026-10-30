#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="$(mktemp -d)"; trap 'rm -rf "$BUILD"' EXIT
FLAGS=(-std=c++20 -O2 -Wall -Wextra -Werror -ISource/DywxUI/Public)
if [[ "${SANITIZE:-0}" == 1 ]]; then FLAGS=(-std=c++20 -O0 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -ISource/DywxUI/Public);export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}";fi
"${CXX:-g++}" "${FLAGS[@]}" Source/DywxUI/Private/MVP/{Simulation,Progression,ContentCatalog,Campaign,Economy,Arena,SaveCodec}.cpp tests/full_game_integration_tests.cpp -o "$BUILD/test"
"$BUILD/test"
