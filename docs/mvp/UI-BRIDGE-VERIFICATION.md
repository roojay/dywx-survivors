# MVP gameplay UI bridge

The controller spawns one `ADywxMVPWorld`, releases preview rendering suppression before starting it, reads its authoritative snapshot at 20 Hz, and sends movement every player tick. Commands refresh immediately. No duplicate gameplay simulation exists in the widget. Entity vectors are not copied to the UI; only progression/player/statistics and boss aggregate are retained.

## Controls and flow

- Main → two heroes → 栖雷古坛 (fire/thunder provisional map label) → Quick only.
- WASD / arrows: screen-consistent movement. Space: dash. 1/2/3: choose the current upgrade card. Clicking cards also works; combat does not pause for offers.
- Esc: explicit simulation pause/resume. Rest is simulation-owned and unlimited.
- Rest: select equipment slot, select bag relic, then 换装. 卸下 returns equipment to bag. Lock applies to selected bag item, or selected equipment if no bag selection. Filters cycle rarity, aspect and affix; sorting is deterministic. Refinement previews all visible unlocked bag items and exact XP; explicit confirmation required; undo works until 迎战. Pending refinement XP is marked and only committed by the simulation on 迎战. Existing queued upgrades must be selected before 迎战.
- Six spell slots with live cooldowns; four dedicated passive slots; health/shield/XP/time/wave/damage/kills/resonance; two-phase boss HP; results and direct retry/menu; optional boss reward selection.
- Catalog uses the same names and effects as authoritative content. Text/portraits are presentation; gameplay enters the rendered world, not a static preview.

## Checks

- `scripts/test-ui-state.sh`: navigation, both heroes, future mode locks, repeat/back/pause policy.
- `scripts/test-preview-budget.sh`: preserved menu budget ownership policy.
- `scripts/test_font_layout.py`: 36 actual Sarasa metric checks at 100% / 110%, including prior 24 representative slots adapted for live HUD and new rest/upgrade labels. This is not Slate layout evidence.
- `scripts/test-mvp-ui-contract.py`: source bridge/stale callback/input/slot contracts, delimiter balance, UI/catalog CJK coverage in both bundled fonts.

**NOT_RUN:** UE/UHT compile, engine execution, Slate visual/input checks, 720p/1080p screenshots, packaging, FPS/GPU verification. No engine/GPU is available here. Windows packaged smoke remains required. Offline checks must not be interpreted as proof of playable or smooth packaged runtime.
