#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="${FULL_GAME_CALIBRATION_DIR:-/tmp/dywx-full-game-natural}"
mkdir -p "$BUILD"
FLAGS=(-O2)
if [[ "${SANITIZE:-0}" == 1 ]]; then FLAGS=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer); fi
COMPILE=(tests/full_game_playthrough.cpp Source/DywxCore/Private/MVP/{ContentCatalog,Progression,Simulation,Campaign,Economy,Arena,SaveCodec}.cpp)
SOURCES=("${COMPILE[@]}" Source/DywxCore/Public/MVP/{Export,Types,Simulation,Progression,ContentCatalog,Campaign,Economy,Arena,SaveCodec}.h scripts/calibrate-full-game.sh)
sha256sum "${SOURCES[@]}" > "$BUILD/source-sha256.txt"
"${CXX:-g++}" -std=c++20 "${FLAGS[@]}" -Wall -Wextra -Werror -Wpedantic -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public "${COMPILE[@]}" -o "$BUILD/playthrough"
"$BUILD/playthrough" "$BUILD/trace.csv" | tee "$BUILD/results.csv"
sha256sum --status -c "$BUILD/source-sha256.txt" || { echo 'STALE: source changed during calibration. Preserve intermediate results, rerun.' >&2; exit 2; }
echo "Natural smoke evidence: $BUILD" >&2
