#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="$(mktemp -d "${TMPDIR:-/tmp}/dywx-mvp-acceptance.XXXXXX")"
trap 'rm -rf -- "$BUILD"' EXIT
BIN="$BUILD/acceptance-${1:-normal}"
FLAGS=(-std=c++20 -Wall -Wextra -Wpedantic -I Source/DywxUI/Public)
if [[ ${1:-} == --sanitize ]]; then FLAGS+=(-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer); else FLAGS+=(-O2); fi
"${CXX:-g++}" "${FLAGS[@]}" tests/mvp_acceptance.cpp Source/DywxUI/Private/MVP/{ContentCatalog,Progression,Simulation}.cpp -o "$BIN"
"$BIN" "${@:2}"
