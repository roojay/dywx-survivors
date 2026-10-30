#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/dywx-arena-tests.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT
FLAGS=(-std=c++20 -O2 -Wall -Wextra -Werror -ISource/DywxUI/Public)
if [[ "${SANITIZE:-0}" == 1 ]]; then
 FLAGS=(-std=c++20 -O0 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -ISource/DywxUI/Public)
 export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
"${CXX:-g++}" "${FLAGS[@]}" Source/DywxUI/Private/MVP/Arena.cpp tests/mvp_arena_tests.cpp -o "$BUILD_DIR/arena"
"$BUILD_DIR/arena"
