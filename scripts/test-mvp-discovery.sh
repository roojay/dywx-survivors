#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp /tmp/dywx-discovery.XXXXXX)"
trap 'rm -f "$bin"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -I Source/DywxUI/Public tests/mvp_discovery_test.cpp Source/DywxUI/Private/MVP/{ContentCatalog,Progression,Simulation,Campaign,Economy,Arena,SaveCodec}.cpp -o "$bin"
"$bin"
