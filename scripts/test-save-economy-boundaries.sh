#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build="$(mktemp -d /tmp/dywx-save-economy-boundaries.XXXXXX)"
trap 'rm -rf "$build"' EXIT
flags=(-std=c++20 -Wall -Wextra -Werror -pedantic)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
"${CXX:-g++}" "${flags[@]}" -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public tests/mvp_economy_boundary_tests.cpp Source/DywxCore/Private/MVP/{ContentCatalog,Progression,Economy}.cpp -o "$build/economy"
"$build/economy"
"${CXX:-g++}" "${flags[@]}" -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public tests/mvp_save_boundary_tests.cpp Source/DywxCore/Private/MVP/{ContentCatalog,Progression,Economy,Campaign,Arena,SaveCodec}.cpp -o "$build/save"
"$build/save"
