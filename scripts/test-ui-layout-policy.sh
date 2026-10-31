#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD=$(mktemp -d); trap 'rm -rf "$BUILD"' EXIT
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic -I Source/DywxUI/Public tests/ui_layout_policy_test.cpp -o "$BUILD/test"
"$BUILD/test"
