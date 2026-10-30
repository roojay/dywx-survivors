# Codex discovery and human fusion action

## Delivered source behavior

- The Codex lists all 12 spell entries (base and rank-five effects), five passives,
  both fusion recipes, 15 relics, eight enemy roles (six ordinary roles, boss,
  spirit beast), four mutations and five resonance rules. Unknown entries remain
  readable grey; discoveries are brighter with an explicit seen/unseen prefix.
- Existing spell/relic/fusion flags are reused. New passive/enemy/mutation/resonance
  flags are optional fields in profile JSON version 1. Missing fields default false;
  merges are monotonic OR. Profile content remains discoveries and pity only.
- Enemy encounter means **successful insertion into the active arena**, including
  off-camera enemies; rejected capped spawns do not discover anything. The sole
  simulation change is a metadata recording call after insertion. It consumes no
  RNG and changes no health, targeting, damage, spawn rules or balance constants.
- Passives light when owned; resonance lights at two counts (forms only when
  enabled). Reset clears discoveries, and host profile import restores flags only,
  not inventory, power, level, experience, survival state or combat state.
- Opening the Codex reads a local saved-profile view even before any world exists,
  merges current-run discovery flags if available, and never writes or spawns.
- A hero-correct fusion button now exposes the existing core fusion command. It is
  available only when ingredients are ready and the player is resting or has a
  pending offer, never paused/dead/won. Button epoch plus generation and live-state
  checks reject stale actions. Success refreshes the snapshot/UI. Pause-state
  snapshot transitions refresh availability after resuming.
- Fusion button bounds `(1360,130,510,80)` avoid the boss bar ending at x1330 and
  upgrade cards starting y225. This is a source-coordinate check, not rendered QA.

## Offline verification

`test-mvp-discovery.sh`: reset, monotonic merges, starter/passive/resonance flags,
all roles/mutation bits, invalid enum rejection, cap rejection, and RNG parity.
`test-mvp-progression.sh`: existing progression/fusion/inventory assertions pass.
`test-mvp-profile-contract.py`: optional version-one fields and persistence scope
source checks (not executable Unreal JSON round-trip).
`test-mvp-ui-contract.py`: actual fusion invocation and guards, saved Codex view,
grey/lit lists, delimiters and 477 CJK glyphs in both bundled Sarasa fonts.
`test-mvp-focus-contract.py`: existing offline focus contracts remain passing.
`test_font_layout.py`: 40 cases, including fusion label and actual 248-pixel inner
upgrade-card width; candidate title height is only checked against the inner-card
bound, not a claimed Slate AutoHeight allocation. Canonical current passive names
already omit book-title brackets; descriptive card text now includes their name.

**NOT_RUN:** Unreal compile/UHT, JSON/disk restart round-trip, rendered grey/lit
state, actual fusion mouse/keyboard/paused/stale input flows, text clipping,
packaged gameplay, GPU/FPS and human-play acceptance. These require UE runtime.
