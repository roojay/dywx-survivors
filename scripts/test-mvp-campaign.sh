#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp /tmp/dywx-campaign.XXXXXX)"
trap 'rm -f "$bin"' EXIT
flags=()
if [[ "${SANITIZE:-0}" == 1 ]]; then flags=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic "${flags[@]}" -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public -I Source/DywxRuntime/Public -I Source/DywxUI/Public tests/mvp_campaign_tests.cpp Source/DywxCore/Private/MVP/Campaign.cpp -o "$bin"
"$bin"
