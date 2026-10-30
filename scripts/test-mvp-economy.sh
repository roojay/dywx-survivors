#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp /tmp/dywx-economy.XXXXXX)"
trap 'rm -f "$bin"' EXIT
flags=()
if [[ "${SANITIZE:-0}" == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
g++ -std=c++20 -Wall -Wextra -Werror -pedantic "${flags[@]}" -I Source/DywxUI/Public tests/mvp_economy_tests.cpp Source/DywxUI/Private/MVP/{ContentCatalog,Progression,Economy}.cpp -o "$bin"
"$bin"
