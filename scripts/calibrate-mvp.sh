#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="${TMPDIR:-/tmp}/dywx-mvp-playthrough"
mkdir -p "$BUILD"
FLAGS=(-O2)
MODE=normal
if [[ ${1:-} == --sanitize ]]; then
  FLAGS=(-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
  MODE=sanitized
  shift
fi
SOURCES=(tests/mvp_playthrough.cpp Source/DywxUI/Private/MVP/{ContentCatalog,Progression,Simulation}.cpp Source/DywxUI/Public/MVP/{Types,Simulation,Progression,ContentCatalog}.h)
sha256sum "${SOURCES[@]}" > "$BUILD/$MODE-source-sha256.txt"
"${CXX:-g++}" -std=c++20 "${FLAGS[@]}" -Wall -Wextra -Wpedantic -I Source/DywxUI/Public "${SOURCES[@]:0:4}" -o "$BUILD/playthrough-$MODE"
"$BUILD/playthrough-$MODE" "$@"
sha256sum --status -c "$BUILD/$MODE-source-sha256.txt" || { echo 'Sources changed during calibration; retain as intermediate evidence only.' >&2; exit 2; }
