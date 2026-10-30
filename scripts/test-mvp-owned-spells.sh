#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp /tmp/dywx-owned-spells.XXXXXX)"
trap 'rm -f "$bin"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -O2 -I Source/DywxUI/Public tests/mvp_owned_spell_tests.cpp Source/DywxUI/Private/MVP/{ContentCatalog,Progression,Simulation}.cpp -o "$bin"
"$bin"
