#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp /tmp/dywx-progression.XXXXXX)"
trap 'rm -f "$bin"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -I Source/DywxUI/Public tests/mvp_progression_test.cpp Source/DywxUI/Private/MVP/ContentCatalog.cpp Source/DywxUI/Private/MVP/Progression.cpp -o "$bin"
"$bin"
