# MVP independent acceptance matrix

Authority: `gameplay-design-v0.1.md` plus numeric `mvp-prototype-config-v0.1.md`. This is not the earlier three-spell preview. Config-specific MVP scope overrides full-game ambitions. Tests use production pure-C++ APIs; no Unreal/GPU claims follow from them.

## Resolved interpretation
- Form resonance is configurable, default ON; exclusive passive occupies one of four slots.
- Refinement XP is pending during rest, undoable before 迎战, committed once on leaving rest; fixed rarity exchange ignores XP bonuses.
- No currency, shops, third-layer systems or endless mode.
- Dao relics contain five ordinary/mystery affixes plus their extra unique effect (the item table is more specific than the generic affix count).
- ThunderSword's explicit nearest-target rule takes precedence over the general sword high-HP preference.
- Six-wave damage scaling is not specified; any interpolation must be labeled implementation calibration.
- Twenty-wave pity needs a persistence owner; exposed getter/setter alone is not persistence across runs.

## Measurable requirements and evidence gates

| ID | Required behavior | Independent acceptance |
|---|---|---|
| LOOP | A=4 waves 100/120/140/160s, B=6 waves 70/75/80/85/90/95s, unlimited frozen rest after each, explicit next, then two boss phases | Default assertions, shortened explicit harness full lifecycle, real-duration seeded runs |
| HERO | Body starts palm + body scripture; Sword starts flame sword + sword scripture | State and derived statistics |
| XP | thresholds 15+8n (first 23), queue multiple levels; choices freeze combat (user revision 2026-10-31) | Boundary grants, frozen snapshots with queued offers, substep stop and resume |
| CHOICE | 3 distinct legal candidates, related candidate, 2 rerolls, six spells/four passives/max rank5 | Seeded offer loops, exhausted build, invalid command rejection |
| FUSE | hero-gated rank5 pairs become one slot; elemental/form counts preserved | Both recipes via real upgrade choices |
| SPELL | ten mechanically distinct skills and all rank2–5 changes | Numeric table plus runtime observations; table existence alone NOT evidence |
| PASSIVE | five passives change stats, affinity, burn/chain/XP as specified | Derived stats and actual combat consumers |
| RESONANCE | fire/thunder 2/4 and optional body/sword/array 2/4 | Counts, toggle, runtime effect probes |
| RELIC | 15 templates, random affixes, four rarities; six autoequipped, overflow bag inert; M1/M2 exclusive | Catalog generation, stats before/after, swap rejection |
| PROC | T1–T7, M1–M6, U1–U2 actually affect combat | Consumer audit + deterministic runtime probes; not just boolean flags |
| BAG | rest-only equip; locks respected in batch refine; deduplicate IDs; preview/undo; pending XP committed once | Repeated/stale IDs, locked items, undo/restart cases |
| DROP | 0.5% normal/2.5% variant, elite/beast guaranteed Spirit+, luck weight not rate, 20-wave pity; boss distinct Immortal/Dao three-choice | Seeded distribution and invariant tests, persistence audit |
| ENEMY | six roles, 4 mutations, summoner max12, lightning exploder friendly-only, fire shield ×2 | Focused actual simulation probes |
| SAFETY | initial 5s grace, dash4 units/0.3s immune, warnings precede harm | Tick-by-tick probes |
| BOSS | 14000+16000 HP, phase1 lightning/shield/adds; phase2 shrinking wall/charge/adds; terminal result | Lifecycle and hazard observations |
| BOUNDS | wave caps40/70/100/130 and B30/50/70/90/110/130; boss80; bounded projectiles/effects/pickups; finite stats | Long seed runs and saturation harness |
| RESTART | clear stale actions/entities/XP/equipment/phase; generation advances | repeated rest/dead/won restart, invalid commands |
| TIME | equivalent elapsed simulation under 30/60/120 FPS; reject NaN/negative dt; pause/rest/terminal freeze | partition replay and mutation-free snapshots |
| VIEW | 2.5D real world, clear telegraphs, distinct actors, Chinese text/complete codex, no invisible blockers | NEEDS UE build and visual playtest; unavailable on dot |
| PERF | 100+ enemies at smooth target framerate | CPU harness timing is diagnostic only; NEEDS target-device GPU/UE profiler |
| FEEL | early death<5%, 18–20 upgrades, 30–40% fusion, boss90–130s, replay intent | autoplay informs calibration, cannot establish human enjoyment |

## Evidence status
Implementation is being written concurrently. Results below will be replaced with measured commands and findings after the API stabilizes. No row is implicitly passed by this matrix.

## Measured evidence (2026-10-30, dot Linux)

Commands:
```sh
bash scripts/run-mvp-acceptance.sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/run-mvp-acceptance.sh --sanitize
bash scripts/run-mvp-acceptance.sh normal --autoplay
```
Toolchain: g++ 14.2.0, C++20, Linux x86_64; `/proc/cpuinfo` identifies AMD EPYC 9V74. This is a shared virtual executor, not target Windows hardware.

Independent O2 suite: **606,530 assertions passed**, about **482 ms total** in the recorded run. The large assertion count is primarily repeated finite/bounds checks, not 606,530 distinct scenarios.

- Default numeric config, both hero starters, XP thresholds23/31/39, queued upgrade combat continuation, exactly two rerolls and invalid choices.
- Both heroes complete explicitly shortened harness A/B wave lifecycle and both real14000/16000 boss HP transitions; rest/dead/won/pause freeze and clean generation-incrementing restart. Boss HP is removed by deterministic test damage, **not a natural victory**.
- Dash4 units/0.3s immunity; initial5s grace; repeated/locked/refined IDs, deduplicated preview, undo and once-only XP commitment.
- 30/60/120FPS fixed-input deterministic replay, including actual spawning/combat. Reactive bots sample decisions per frame, so their trajectories are not expected to be identical across FPS.
- Every one of12 spell IDs creates damage/projectiles/effects, and focused probes observe SwordRain8 warnings, HeavenlyThunder rank5 three delayed strikes, lightning-friendly exploder chain, M1 dash fire, M2 endpoint damage, M3 fifth-launch splits, M4 moving reduced-radius field, U2 resonance counts, T1 speed, T3 dash power, T4 healing and T7 low-HP armor.
- Sustained stress: seed991, **36,000 ticks /600 simulated seconds**, max160 enemies, cap128 each projectiles/effects/pickups, 50,138 bounded dropped events, **477ms** recorded simulation wall time. Harness explicitly restores HP each second and casts all skills; this is saturation/safety evidence, **not balance/playability evidence**. Dropped events are counted, not unbounded allocations.
- ASan/UBSan run status is recorded below. LeakSanitizer could not run under the host's ptrace environment; disabling leak detection does **not** establish absence of leaks.

### Behavior audit and fixes requested
Runtime consumer inspection found and reported these mismatches, subsequently addressed by the combat owner: A damage multipliers, rank5 ThunderFist high-HP chain, ThunderSword turning multiplier, FlameSword ground trail, FusionSwords initial orbit, prison-local vulnerability, M6 other-spell-only cooldown reduction, M3 actual launches rather than empty casts, and sub-frame dash edges. These are not merely catalog entries. Rare chance-dependent proc rates T2/T5/T6, precise full-combination numerical balance and U1/M5/M6 temporal interactions are source-audited but **not independently isolated statistically by this acceptance suite**. The combat owner's separate suite supplies additional evidence; do not conflate its tests with this suite.

### Shipping-duration calibration, not certification
18 runs: seeds11/29/73 × Body/Sword ×30/60/120 FPS, shipping wave durations, no HP/damage overrides. A full-speed avoidance bot reaches wave4 or boss as Body but only4–7 kills/level1 in most runs because it faces away while fleeing; Sword reaches255–1076 kills and levels5–14 but dies in waves2–4. No wins. An earlier slow ellipse bot died in25–31 seconds. These bots do not prove human early-death rates, fun, fusion probability or intended boss completion time; they show that survival-only autopilot is not an adequate skill/fusion calibration policy. Do not retune shipping numbers solely to make this bot win.

### Still requiring target execution
UE compilation/linking, packaged launch, visible warning-before-hurt readability, Chinese font/layout in actual game, frame pacing, GPU memory/load, audio and enjoyable human runs remain unverified without UE/Windows/GPU. Pure-C++ timings cannot certify a smooth or polished game. Full numeric codex and persistent pity integration are separate UI/profile acceptance gates.

Sanitizer evidence: `ASAN_OPTIONS=detect_leaks=0 bash scripts/run-mvp-acceptance.sh --sanitize` passed606,530 assertions,8084ms including8031ms saturated stress. No AddressSanitizer/UndefinedBehaviorSanitizer diagnostics. LeakSanitizer remains unverified.

Independent boss population regression reproduced a failure: with79 actors immediately before phase1 death, transition spawned beyond80. Core owner notified; final rerun below supersedes this if fixed. Retaining this note makes the audit trail explicit.

### Latest independent rerun
- Boss population cap regression is now **fixed and passed**.
- Added all10 base spells at each rank1–5, asserting runtime snapshot changes (not catalog-only). Explicit checks cover damage rank2, palm burn duration/afterwave, flame sword count/pierce/return, thunder sword chains/turn speed/dual distinct targets, rain count/afterfield, array radius/duration/following, prison stun/radius/local-vulnerability flag, and heavenly warning/afterbolts. FlameBody rank4 cadence/rank5 owned-build hurt response and Ring rank4 time-dependent rotation remain delegated to the combat suite.
- **607,462 assertions passed O2 and ASan/UBSan** after boss-cap correction; sanitized7188ms. Subsequently disabled automatic casting in isolated fixtures using the explicit harness-only config (production default remains true): **607,402 assertions passed**,504ms O2, including499ms stress,41,728 capped events. Count changes reflect different entity populations, not removed test categories.
- Test source is `tests/mvp_acceptance.cpp`; all fixture modifications use public production progression/simulation APIs or explicitly documented test hooks. No tests alter shipping defaults.

### Tactical autoplay follow-through (untuned shipping defaults)
Preserved logs: `calibration-avoidance.csv`, `calibration-tactical.csv`, `calibration-pulse-pre-cap-fix.csv`, `calibration-pulse.csv`, `calibration-observant.csv`. These are successive code snapshots, not controlled balance experiments: combat bug fixes landed during testing. No shipping duration/HP/damage overrides were applied. `phase=5` means Dead; `bossPhase=1` establishes boss reachability even though `wave` still reads4.

- Deliberate turn-in/hold facing, dash-through at1.3 units, nearby XP pursuit, AoE-first upgrades and rarity-improving rest swaps still produced0/18wins, mostly waves1–2. Standing to attack invited surround pressure.
- Pulse turn-in/full-speed kite reached1439kills/level19 as Sword (seed11,120FPS), boss at538.9s, but no victory. It exposed a natural max148 actors despite wave130 cap; core fixed cap enforcement for summoner/split sources centrally. The explicit TestSpawn stress hook alone can bypass phase cap up to configured global limit.
- Observant policy reads visible palm slash effects to schedule turn-ins before the next automatic cast. Body improves to70–910kills/levels2–13; four of9 Body runs reach boss, still no first-phase kill. This shows naive pulse timing drift was a bot defect, not evidence that melee cannot deal damage. `--observant` remains a diagnostic agent, not a human substitute.
- Mechanical timing probe: Body with target in front on first tick deals at least16 damage. But empty cast at start, target entering2.5 units at0.1s, then0.7s advance yielded0damage dealt and4.92received: full cooldown consumed by empty melee casts lets an approaching swarm reach contact before first hit. Reported separately to combat owner; any subsequent fix is a targeting-latency correction, not a numerical damage buff.

Reproduce policies with `bash scripts/run-mvp-acceptance.sh normal --tactical`, `--pulse`, or `--observant`. A natural full victory has **not** been demonstrated; full lifecycle harness victory is intentionally labeled test-damage-assisted.

### Final safety and default-parameter rerun (16:45 UTC)
- Formatted `tests/mvp_acceptance.cpp` with the parent-provided clang-format tool; no source implementation files edited by this audit worker.
- **607,413 assertions passed O2**,414ms; stress150 peak actors,42,543 safely capped events,410ms. Peak differs from earlier160 because newly enforced phase caps also constrain actual summoned children (the explicit stress fixtures still inject150).
- Added240FPS short-pulse input regression: a dash pressed/released before one60Hz simulation tick is retained and executed once.
- AddedmaxEffects1 warning saturation regression: a charger owns the sole live hostile telegraph; a newly close exploder cannot hurt invisibly while its warning cannot be admitted. Passed after core admission/cancellation fix.
- Body empty-cone latency fix passed: same0.7s front-entry probe now deals18 and receives0, with explicit regression assertions.
- Final natural-play log `calibration-observant-final.csv`:18 runs, **no victories**, six reach first boss phase; four remain alive at850 simulated seconds (the deliberately bounded calibration observation window, not a terminal result). Best1347kills/level17. All observed natural populations≤130. This demonstrates reachability but not boss defeat, intended boss DPS, or human playability. No production-number tuning was performed by this audit.

Final exact-source sanitizer rerun: **607,413 assertions passed ASan+UBSan**,5668ms (5613ms stress), no diagnostics; `detect_leaks=0` still required by host ptrace. Source fingerprints at `acceptance-source-sha256.txt` identify this audit snapshot; rerun after any later code changes. `git diff --check` passed.
