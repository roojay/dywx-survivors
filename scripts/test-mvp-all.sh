#!/usr/bin/env bash
# Offline source/logic/resource gates; this script never labels Unreal checks passed.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/test-ui-state.sh
bash scripts/test-ui-layout-policy.sh
bash scripts/test-ui-view-memory.sh
python3 scripts/test-ui-view-memory-contract.py
bash scripts/test-preview-budget.sh
bash scripts/test-mvp-progression.sh
bash scripts/test-mvp-discovery.sh
bash scripts/test-mvp-combat.sh
bash scripts/test-mvp-owned-spells.sh
bash scripts/run-mvp-acceptance.sh
python3 scripts/test_font_layout.py
python3 scripts/test-mvp-ui-contract.py
python3 scripts/test-battle-hud-contract.py
python3 scripts/test-concept-assets.py
python3 scripts/test-concept-ui-contract.py
python3 scripts/test-mvp-profile-contract.py
python3 scripts/test-mvp-focus-contract.py
python3 scripts/validate_assets.py
python3 scripts/validate-mvp-render-contract.py
python3 scripts/test-mvp-art-material-contract.py
python3 Art/Blender/MVP/validate_geometry.py
python3 -m py_compile scripts/bootstrap-mvp-assets.py scripts/test_font_layout.py Art/Audio/build_sfx.py
if [[ "${1:-}" == "--sanitize" ]]; then
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" bash scripts/run-mvp-acceptance.sh --sanitize
fi
printf '\nPASS: available offline gates. NOT_RUN: Unreal build/cook/launch/render/GPU/human-play acceptance.\n'
