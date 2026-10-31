# Isolated gameplay pacing diagnosis — 2026-10-31

Source baseline: 2e5eb1e0f5a89779d8936eda7f4ca943ee686d69. Independent `/tmp/dywx-pacing-experiment`; no production source edits. Pure C++ headless simulation at 60 Hz, controls updated at 10 Hz, observation bound 1200 simulated seconds. Reuses `tests/full_game_playthrough.cpp` keyboard-like legal-input AI, legal shop/rest/equip/fusion/upgrade commands. No HP, currency, XP, skill injection. These are automated policy probes, NOT player win rates or evidence that gameplay is fun.

## 中文摘要

本轮优先验证玩法而非继续换框架。发现自动策略选中经验后仍保持战斗距离，修正的是测试走位语义，未修改生产战斗数值。激进捡取和全局减半 Boss 血量都没有提供一致收益，未合入游戏。所有失败/超时保留；当前新号八例全部自然死亡，默认十例为1胜8死1超时。它们不是人的胜率，也不证明好玩或不好玩。下一步以两初始英雄、雷泽、快速模式的真人可玩闭环为准。

## Matrix and policy versions
- Quick mode; seeds fixed in advance: 11, 29, 73, 101; three heroes in ThunderMarsh.
- `tier1_minimal_hero_unlock`: genuine new Body/Sword account; Beast has ONLY required hero unlock, explicit fixture, no permanent stats or other content unlocks.
- `all_content_zero_stats`: all content unlocked, zero permanent stats.
- Baseline p0 and p10: 48 cases + Beast MistForest seed73 sentinel under both policies =50.
- p0 is existing preference/range policy. p10 combines starter-pair preference (+30 choice score) with nonmelee Boss desired distance3.2 instead6, so it is NOT a one-factor comparison.
- p20: p0 + nearest XP within12 attracts heading score with weight4, regardless of nearest enemy. Legal movement only; intentionally diagnostic and unsafe, not recommended.
- p21: p0 + preserves original XP selection gate (no Boss, nearest enemy>4, nearest XP within7), fixes selected-XP desired distance to0.5 rather than combat distance2.5/4 and scores selected XP even without enemy target. Legal movement only.
- Boss-half: baseline policies; only `RunConfig.bossHp1/2` scaled together by0.5 (14000/16000 ->7000/8000). Veteran and sentinel26-case matched subset only.
- p30: Beast-only diagnostic, same movement as p0, existing upgrade policy2 adds25 score to any new-spell offer, favoring breadth over repeats. Eight cases complete:0 won,3 dead,5 bound; still no fusion. Mere new-spell score bias does not fix severe Beast growth stalls.

## Results
|Variant|Cases|Won|Dead|1200s bound|
|--|--:|--:|--:|--:|
|baseline p0+p10|50|5|31|14|
|baseline p0 matched pickup subset|25|2|17|6|
|aggressive pickup p20|25|1|21|3|
|safe-gate pickup correction p21|25|2|19|4|
|baseline veterans matched HP subset|26|5|14|7|
|Boss HP half|26|6|13|7|

All baseline deaths occur after180 seconds, but four seeds/cell and a full-snapshot AI cannot establish the design's human pre3min death-rate target.

Fresh Body11 baseline: first upgrade85.1537s with108 kills and85XP lying on ground; wave end100.007s consumes ground XP and jumps level2->5. First two-fire resonance85.1704s. Dies215.791s wave2, level8, no rank5 or fusion,115XP on ground. p21 survives496.273s and first fusion464.164s but still dies. p20 instead dies26.6331s level1: chasing early XP is not automatically beneficial.

Beast Thunder stalls differ from the Mist sentinel: baseline veteran Beast11 p0 is onlylevel5/133kills after1200s; p10 onlylevel4/118kills. No two-element build or fusion. At539.9s BossHP13976.4/14000, at659.9 still13976.4 with40shield, mobs capped80. Beast73 p10 at1170s stillBossHP13792, level4. These are severely low damage/low growth trajectories, not merely an oversized final health pool. Boss applies40shield to OTHER mobs every15s; Support can shield three nearby other units including Boss. Do not misreport Boss passive regeneration.

Boss-half brings surviving Body fights to roughly82–110s, near configured90–130s goal, but gives only one additional win, leaves Beast Thunder bounds, and Beast Mist73 p10 stilltakes199.4s vs490.8s original. Its earlier victory also prevents the original948.9s fusion from occurring. Blanket half-HP is not currently justified as the production fix.

## Recommended next decision
1. Keep production numbers unchanged until an actual playable human test. Add these timing/choice observations to a repeatable diagnostic runner; fix test-AI pickup semantics separately from production balance.
2. Surface early available XP/skill upgrades and build goals in UX; test the trade-off of approaching XP under danger rather than silently assuming a skilled AI proves early progression healthy.
3. Investigate Beast early single-target growth, Support/shield target selection and lack of area damage. Compare legal build breadth policies first; do not grant abilities or inflate stats to force a pass.
4. After early builds are healthy, isolate Boss HP budget by hero/build DPS and compare target encounter duration. Quick wave schedule itself already consumes520s; overall8–12min target leaves only~80–200s for Boss excluding menus.

## Evidence/reproduction
CSV includes firstupgrade, firstrank5, firstfusion, first2element, death/wave, Bossentry/duration, damageTaken, uncollectedXP, final build. `first_2_element` specifically means the first elemental resonance count reaching2 (two sources of the SAME element), NOT acquiring two distinct elemental types. Negative timing=-1 means not observed before death/bound. Boss_seconds is time observed since Boss entry, not necessarily a completed encounter. EventCSV records choice IDs and milestone XP/level. Choices use original engine enum IDs, not invented prose labels.

Compile each stored harness against unchanged core:
```
g++ -std=c++20 -DDYWXCORE_STANDALONE -O2 -ISource/DywxCore/Public Source/DywxCore/Private/MVP/{Simulation,Progression,ContentCatalog,Campaign,Economy,Arena,SaveCodec}.cpp HARNESS.cpp -o OUTPUT
```
Baseline: `evidence/baseline-harness.cpp`; greedy: `evidence/greedy-pickup-harness.cpp`; safe: `tests/full_game_playthrough.cpp`; trace: `tests/pacing_boss_trace.cpp`; breadth: `tests/pacing_buildbreadth.cpp`.
CLI args: bossHPScale profileFilter eventsOutput; filter -1=all,0=veteran,1=tier1. Capture stdout CSV. Trace additionally emits diagnostic rows to stderr: sample,elapsed,bossphase,bosshp,shield,distance,damageDealt,kills,level,enemycount,build.

## Minimal production harness correction (approved and applied)
`minimal-natural.patch` changes only `tests/full_game_playthrough.cpp`: Simulation-level Fuse/Equip, safe-gate XP policy correction, first-upgrade/fusion/ground-XP/choice observability, optional `--quick-onboarding` (Body and Sword, ThunderMarsh Quick, fresh profile, four fixed seeds each). Default original ten-case matrix retained. Output `policy=0` retains its original actual strategy meaning. New `harness_revision=pickup-distance-v1` identifies the pickup correction; experimental p21 uses this same safe pickup movement. No production core, balance, content or UI edits. `first_upgrade` is the first observed level>1 timestamp, not the time a human or AI chooses an offer.

Run after compilation: `OUTPUT UNIQUE_TRACE.csv --quick-onboarding > UNIQUE_SUMMARY.csv`; default ten cases omit selector. Never overwrite older policy0 evidence. Ordinary-run results: onboarding8 cases all died; original default10-case matrix under pickup-distance-v1 produced1 win/8 deaths/1 bound. The previous policy implementation on core2e5 produced3 wins/4 deaths/3 bounds. These use different movement policies; this is NOT a production regression comparison or evidence the game became stronger. Recommendations: compile +8 onboarding cases +original10 cases + existing core command/integration suites; evaluate legal command success/invariants rather than requiring wins. Finite-sample deaths and caps are retained, not hidden.

### Archived experimental source deltas
`reproduction/baseline-from-2e5.patch` applies to the original natural harness at2e5eb1e, creating the isolated50-case experiment. Each other reproduction patch applies independently to that experimental baseline (not cumulatively). Thus CSV results remain reproducible without introducing another production framework. Boss-half reuses the experimental baseline executable with CLI `0.5 0 EVENTS.csv`. Do not apply these archived experiment patches to the production harness.
