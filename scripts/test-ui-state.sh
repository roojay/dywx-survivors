#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD_DIR="${TMPDIR:-/tmp}/dywx-ui-state-tests-$$"
mkdir -p "$BUILD_DIR"
trap 'rm -f "$BUILD_DIR/ui_state_test"; rmdir "$BUILD_DIR"' EXIT
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic -I Source/DywxUI/Public tests/ui_state_test.cpp -o "$BUILD_DIR/ui_state_test"
"$BUILD_DIR/ui_state_test"
