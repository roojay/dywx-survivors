# Actual-core natural playthrough calibration

Final calibration against the reviewed combat core (Simulation SHA256 `6ccdd2b9ac05b40bd4ad57030be584e7e6a5fe3985bd79d91125ec9d34fe5e97`).

This is **pure C++ `Simulation` evidence**, not Unreal execution, target-device FPS, human usability, or enjoyment certification. Shipping numbers were not changed to make the bot win.

## Reproduce

```sh
bash scripts/calibrate-mvp.sh 1800 > docs/mvp/calibration-playthrough.csv
bash scripts/calibrate-mvp.sh --sweep 1 24 > docs/mvp/calibration-playthrough-seed-sweep.csv
bash scripts/calibrate-mvp.sh --benchmark > docs/mvp/calibration-dps.csv
bash scripts/calibrate-mvp.sh --trace docs/mvp/playthrough-winning-trace.json 12 1 0
ASAN_OPTIONS=detect_leaks=0 bash scripts/calibrate-mvp.sh --sanitize --trace /tmp/mvp-sanitized-trace.json 12 1 0
```

Natural runs are bounded to 108,000 60Hz iterations (1800 seconds nominal; float elapsed accumulation and frozen rest transitions can differ slightly). A bound is reported `survived_bound`, **never** a victory. `won` requires the production terminal phase. All recorded current runs actually terminated before the bound.

## Natural-run contract

- Unmodified `RunConfig{}`: four waves100/120/140/160 seconds; boss14000+16000 HP; actual spawns, damage, drops, XP, cooldowns, regeneration, and deaths.
- No `Test*` calls, injected XP/relics, player HP restoration, or enemy manipulation in `natural()`. The separately named benchmark below deliberately does use fixture state.
- Input is one of8 normalized keyboard directions or hold, plus legal dash, reconsidered at10Hz and simulated at60Hz. Holding preserves facing. No independent aim, teleport, or scripted damage.
- The agent reads snapshot positions, visible hazard geometry/warnings, and current spell cooldown cues. It predicts short movements to avoid contact, projectiles, charge lines, lightning and fire walls. This idealized telemetry access is **not** a human reaction/visibility test.
- Upgrade policy0 chooses early AoE, fire affinity and starter fusion; policy1 prioritizes the two fusion ingredients more heavily; policy2 favors acquiring additional spells. It uses at most the actual two rerolls, selects actual offers, and calls the legal `Fuse` command while an offer or rest is open. IDs in logs map to `Types.h`; `R` means reroll, `F10/F11` means fusion, other entries are `UpgradeKind:id`.
- At frozen rests, actual dropped bag items are compared via `PreviewEquip`, legal improving swaps are made, and remaining unlocked bag items are legally refined for fixed exchange XP. No inventory is fabricated. Upgrades continue during combat rather than pausing it.
- Boss targeting overrides ordinary nearby targets. Body approaches to melee/fusion range and turns in when palm/fist cooldown cues permit. Sword uses6-unit boss orbit; policies3/4 use3.2/4-unit orbit and closer wave engagement, otherwise upgrade policy0. Body policies3/4 are intentional identical control repetitions, **not independent samples**.

## Results and interpretation

`calibration-playthrough.csv` includes30 runs (seeds11/29/73, both heroes, five policies). Body policy0 is repeated as controls3/4: three winning rows represent **one independent trajectory**, Body73. The separately declared `--sweep 1 24` runs48 cases (24seeds × both heroes, policy0);14 reach boss and4 win. This bounded search demonstrates reachability, not a population win-rate estimate.

**Natural full victories are demonstrated for both heroes**:

| Seed / hero / policy | Total seconds | Boss fight seconds | Final HP | Kills | Level |
|---|---:|---:|---:|---:|---:|
|73 / Body /0|663.815|143.881|17.7358|1538|20|
|12 / Sword /0|636.272|116.338|97.1769|1674|22|
|15 / Body /0|689.392|169.458|12.4078|1509|20|
|18 / Body /0|675.979|156.045|77.2048|1459|19|
|23 / Body /0|679.228|159.294|134.022|1566|20|

All enter the boss at519.934 elapsed seconds and remove the actual30000HP across two phases. The Sword12 win is captured in the final trace and fits the90–130s boss target; it uses FlameSword5, FlameBody5, FlameArray5, FlameRing2, SwordRain1, HeavenlyThunder2 plus its exclusive and Fire passives—**not fusion**. Body wins use legal FusionPalm. All equipment comes from actual drops and legal rest handling.
The Body wins exceed the design90–130s boss window; reaching a terminal win is not proof the target pacing is broadly calibrated. Prior avoidance-only bots' zero boss kills were significantly a **bot engagement problem**, not evidence the implementation makes victory impossible. The old acceptance bot also scored a hypothetical fusion offer but never called the dedicated legal `Fuse` command; `LegalOffers` does not emit fusion offers. The new agent uses that command and can defeat both phases without damage/HP cheats. Conversely, many seeds still die in waves3–4, and overly early fusion specialization can sacrifice required crowd clear. No sample here establishes the human early-death target, intended fusion frequency, or replay intent.

`calibration-playthrough-pre-review.csv` and `playthrough-winning-trace-pre-review.json` preserve the earlier Body11 result (668.164s) before reviewed combat correctness fixes. They are explicitly historical, not the current build. `calibration-playthrough-16heading.csv` is a preserved exploratory analog-heading run from an earlier source snapshot, not the final8-key policy. Its Body29/policy1 win was656.934s. Other transient results changed as separate combat correctness fixes landed; do not pool snapshots into a controlled experiment. `playthrough-source-sha256.txt` identifies the final recorded source set after core stabilization. The run script checks sources have not changed during compilation/execution and returns failure if they did.

## Separately controlled DPS experiment

`--benchmark` is **not natural gameplay**. Each fixture grants XP for exactly19 legal upgrade selections, uses the same seeded offer/choice policy, performs legal fusion at fixture rest, and tests the resulting build for120 seconds. It injects one high-HP (`100000`) Boss at2.5 or6 units and stuns it for the experiment to suppress movement, attacks, shields and adds. All measured damage is from automatic production spell execution, not `TestDamage`. The fixed target and permanent stun are optimistic (including interaction with prison vulnerability); this is not effective moving-boss DPS. No HP restoration is required.

Two explicitly labeled equipment conditions:

- `controlled_no_relic`: no equipment.
- `controlled_six_spirit`: generated legal catalog items4,5,6,7,8,4 (zero-based; two fire bells plus thunder bead, sword tassel, turtle bracer, mobility sash), six slots, no Immortal/Dao. This is a plausible legal loadout **injected for comparison**, not a claim of its drop frequency or a natural acquisition.

Across seeds11/29/73 and policies0/1/2, measured min / median / max DPS:

| Equipment | Hero | Distance | DPS |
|---|---|---:|---:|
| None | Body |2.5|151.2 /193.7 /221.3|
| None | Body |6|0.0 /34.8 /75.1|
| None | Sword |2.5|149.1 /172.1 /189.6|
| None | Sword |6|87.6 /109.2 /122.1|
| Six Spirit | Body |2.5|206.0 /268.0 /310.1|
| Six Spirit | Body |6|0.0 /51.8 /104.3|
| Six Spirit | Sword |2.5|203.6 /235.6 /262.5|
| Six Spirit | Sword |6|127.0 /156.8 /172.8|

The design's roughly300DPS is attainable for some close-range19-choice legal builds with six Spirit items, but is **not a universal property** of either hero or arbitrary range. Six spells at ranks4–5 also require more upgrade expenditure than many actual19-choice builds can afford after passives/fusion ingredients. Melee/AoE range exclusion and auto-target diversion into adds explain substantial effective-boss-DPS loss; this is a calibration finding, not authority to alter shipping values. Controlled numbers alone do not isolate every spell's correctness; separate owned-spell/acceptance tests do that.

## Trace contract

`--trace PATH [seed=12] [hero=1] [policy=0]` runs the actual natural policy and writes JSON schema`dywx-mvp-trace-v1`. `frames` is a ring buffer capped at100 snapshots sampled10Hz, with a final terminal snapshot. It captures approximately the last10seconds, not an unbounded recording. Each frame stores time/phase/bossPhase, player position/HP/facing, enemy IDs/roles/tier/position/direction/radius/HP/maxHP/windup, projectile IDs/source/position/radius/hostile, and effect IDs/kind/source/element/position/end/radius/warning/life/duration/hostile. Enum values are exactly `Types.h`. A renderer must consume this evidence without inventing gameplay and label any Blender render as **offline preview of core telemetry, not Unreal footage**.

## Remaining limits

No UE/GPU execution, visual telegraph readability, real input handling, packaged asset/audio loading, human fun, or target-device performance has been established here. A passing benchmark or one natural victory cannot replace those gates. Both heroes can complete naturally, but robustness across seeds and consistently90–130s effective boss pacing remain open calibration work; no numeric tuning was done by this worker.

## Final validation

- O2 final winning run was replayed twice from the same binary: terminal CSV and all 100 JSON frames were byte-identical.
- ASan + UBSan (`-O1`, `ASAN_OPTIONS=detect_leaks=0`) replayed the final Sword seed 12 / policy 0 victory with no diagnostics. Its result and trace were also byte-identical to the final O2 run. LeakSanitizer is not established under this host's ptrace limitation.
- All 78 natural runs reached a real terminal phase inside the declared bound; matrix controls and seed 11 overlap must not be counted as independent samples. All 72 benchmark fixtures passed exact 19-upgrade / single surviving target / legal slot-count invariants.
- Static audit confirmed the natural loop contains no test hooks, injected XP/relics, or mutable fixture access. Source fingerprints matched after execution; `git diff --check` passed.
- `playthrough-winning-trace.manifest.json` binds the exact trace hash, compiler/build flags and all relevant C++ source/header hashes. This is the source of truth for preview provenance; historical trace names explicitly contain `pre-review`.

After Codex discovery-only metadata additions, all 78 natural results, all 72 DPS rows, and the winning trace were rerun and remained byte-identical. Final fingerprints and trace manifest include those additions.
