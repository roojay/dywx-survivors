#!/usr/bin/env bash
# Full-design offline gate. Does not run UHT, UBT, Cook, or Unreal.
set -euo pipefail
cd "$(dirname "$0")/.."
SOURCE_MANIFEST="$(mktemp "${TMPDIR:-/tmp}/dywx-full-gate-sources.XXXXXX")"
trap 'rm -f "$SOURCE_MANIFEST"' EXIT
find Source Config tests scripts -type f \( -name '*.h' -o -name '*.cpp' -o -name '*.cs' -o -name '*.ini' -o -name '*.py' -o -name '*.sh' -o -name '*.ps1' -o -name '*.json' \) -print0 | sort -z | xargs -0 sha256sum > "$SOURCE_MANIFEST"
if [[ "${1:-}" == --sanitize ]]; then
  export SANITIZE=1
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
bash scripts/test-mvp-all.sh "${1:-}"
bash scripts/test-full-content.sh
bash scripts/test-full-spell-behaviors.sh
bash scripts/test-mvp-campaign.sh
bash scripts/test-mvp-economy.sh
bash scripts/test-mvp-arena.sh
bash scripts/test-full-game-integration.sh
bash scripts/test-mvp-save.sh
bash scripts/test-core-commands.sh
bash scripts/test-upgrade-pause.sh
python3 scripts/test-architecture-contract.py
bash scripts/test-combat-boundaries.sh
bash scripts/test-save-economy-boundaries.sh
bash scripts/test-simulation-design.sh
bash scripts/test-opportunity-flow.sh
bash scripts/test-account-pity.sh
bash scripts/test-damage-feedback.sh
bash scripts/test-checkpoint-replay.sh
sha256sum --status -c "$SOURCE_MANIFEST" || { echo 'Sources changed during this run; this is intermediate evidence, not a final full pass.' >&2; exit 2; }
printf '\nFull-design offline gates passed; UE compile/cook/runtime/performance remain NOT_RUN.\n'
