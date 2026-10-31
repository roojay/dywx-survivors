#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp /tmp/dywx-owned-spells.XXXXXX)"
trap 'rm -f "$bin"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -O2 -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public tests/mvp_owned_spell_tests.cpp Source/DywxCore/Private/MVP/{ContentCatalog,Progression,Simulation,Campaign,Economy,Arena,SaveCodec}.cpp -o "$bin"
"$bin"
