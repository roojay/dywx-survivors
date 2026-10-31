#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp /tmp/dywx-full-content.XXXXXX)
trap 'rm -f "$out"' EXIT
FLAGS=(-std=c++17 -Wall -Wextra -Werror -pedantic -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  FLAGS+=(-O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
${CXX:-c++} "${FLAGS[@]}" Source/DywxCore/Private/MVP/ContentCatalog.cpp Source/DywxCore/Private/MVP/Progression.cpp tests/full_content_tests.cpp -o "$out"
"$out"
