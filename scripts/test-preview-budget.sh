#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/dywx-preview-budget.XXXXXX")"
trap 'rm -f "$BUILD_DIR/preview_budget_test"; rmdir "$BUILD_DIR"' EXIT
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic -I Source/DywxUI/Public tests/preview_budget_test.cpp -o "$BUILD_DIR/preview_budget_test"
"$BUILD_DIR/preview_budget_test"
echo 'PASS: preview foreground/background/minimized caps and stricter user limits (policy only, not UE/GPU validation)'
