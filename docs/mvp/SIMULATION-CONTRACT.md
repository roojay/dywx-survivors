# Deterministic MVP combat contract

Source: `Source/DywxUI/{Public,Private}/MVP/Simulation.*`, paired with
`docs/gameplay-design-v0.1.md` and `docs/mvp-prototype-config-v0.1.md`.

## Ownership and units

- `dywx::mvp::Simulation` owns the run. Render/UI consumers read `GetSnapshot()`;
  they never mutate entity arrays. All lengths are grid units, arena x ±48/y ±27.
- `Start(seed, hero, config)` and `Restart()` reset entities, clocks, combat RNG,
  cooldowns, temporary buffs, inventory/progression and all per-run counters.
  `generation` increases; IDs are unique within that generation. Renderer keys
  must include generation or clear all cached instances when it changes.
- Progression uses a distinct seed stream. No wall clock or `random_device` is used.
  Fixed-input replay is deterministic on the same compiler/platform; no claim of
  cross-platform bit-identical float arithmetic.
- Default simulation step is 1/60 s. Frame dt is finite/positive checked and clamped
  to .25 s; at most 64 substeps are performed. Excess stalled-frame time is not
  simulated. Short render-frame dash press edges are retained until a substep.
- Dash is four units over .3 s with .3 s immunity, toward last movement-facing;
  normalized movement is five units/s before attributes. Cooldown is stats-driven,
  at least two seconds. Obstacles and boundaries constrain movement/dashes.

## Lifecycle

Default A is **100/120/140/160 seconds**, four waves, each followed by unlimited
Rest. `BeginNextWave()` alone leaves Rest; after the fourth rest it starts the
14,000 + 16,000 HP two-phase boss. `RunConfig::SixWave()` supplies B durations
70/75/80/85/90/95 and B HP/caps; B damage interpolation is 1/1.1/1.2/1.3/1.4/1.5.
The B document does not specify individual enemy counts, so A ecological stages
are mapped over six waves without claiming a calibrated B encounter table.

Offers and level-ups **do not pause combat**. Explicit Pause, Rest, Dead and Won
freeze simulation. Existing queued offers can be chosen while moving. Refinement
XP commits on leaving Rest and may queue another live offer. `BeginNextWave` does
not implicitly select upgrades. A wave boundary collects all dropped XP/relics,
clears surviving enemies/projectiles/effects, and grants no kill reward for
surviving enemies. Boss phase transition does not grant victory/rewards; phase-two
kill creates three stable reward choices. Player death takes terminal precedence.

## Combat implementation and bounded work

- Configured six enemy roles, fleeing spirit beast, four elite mutations, two
  boss phases, telegraphed enemy attacks, shield/fire and exploder/thunder rewards.
- Enemy budgets use the A table per-role totals, released across six opening
  segments and subject to simultaneous caps. Spawn grace is five seconds at run
  start. A full simultaneous cap can prevent the theoretical full-clear count.
- Rank configuration comes from `ContentCatalog::GetSpellConfig`, not UI labels.
  Spells implement cones, direct/body/orbit hits, swept projectiles, homing,
  returning swords, chains, timed areas, telegraphed rain/lightning and DoT/control.
- Fusion swords orbit/interweave for .3 s before homing. Flame-sword path burns are
  separate short-lived fields. Rank-five prison vulnerability is spatially scoped.
- Secondary procs cannot recursively trigger primary-hit procs; nested damage is
  bounded to depth three. Dead entities are removed before death explosions/splits.
- Defaults cap enemies160 (wave cap40/70/100/130; boss cap80), projectiles512,
  effects512, pickups1024. Effect/projectile overflow is counted and dropped.
  Production summoner children and elite splits obey phase caps too; only the
  explicit TestSpawn harness hook can bypass a phase cap (never the global cap).
  Pickup overflow immediately banks rewards rather than losing them. Per-run bag
  inventory is intentionally not capped. IDs remain stable across vector erasure.
- Pending hostile warnings have priority over non-hostile visuals/already-fired fields.
  If the effect budget cannot safely admit a warning, its enemy cancels that windup
  and retries, rather than executing an invisible attack. Boss damage fields cannot
  deal damage when their effect is not admitted.
- Permanent FlameBody/FlameRing pulse intervals stay at the rank-table .5/.4 s;
  cooldown reduction applies to casts, not these permanent pulse intervals. Empty
  facing-cone melee casts retry after .15 s instead of wasting their full cooldown.
  Sword burst begins at eight seconds and ring expansion at six, not at run start.
- Body and fire stat resonance bonuses are derived by Progression once; Simulation
  does not multiply an already-derived bonus again.
- UI inventory commands use `GetProgression()` then `RefreshProgression()`;
  Progression itself enforces rest-only equipment/refinement restrictions.

## Tests and known non-deliverables

`tests/mvp_simulation_tests.cpp`: deterministic frame partitions, dash input and
immunity, death/restart, pause/rest freeze, four-wave/boss/win, charge warning,
projectile impact, bounds/obstacles, caps, and live upgrade choices.

`tests/mvp_proc_tests.cpp`: 200 fixed-seed checks for T2/T5/T6/M5/M6, U1 timing,
source-cooldown exclusion, boss cap and terminal reward/clear behavior. Independent
acceptance tests additionally inspect every base spell at ranks1–5 and other procs.
Harness-only overrides (`spawning`, `automaticSpells`, durations, boss HP and caps)
are not shipping input options and defaults retain document values.

The map currently uses **six fixed collision obstacles**, shared with the renderer;
random tile assembly and bespoke spirit-beast escape routes are deferred, not
silently claimed as procedural generation. Enemy avoidance is circle push-out,
not a navigation mesh. Support/summon windups, projectile speed/hit width, boss
fire-wall lifetime/shrink speed and fusion-sword orbit duration are explicit
prototype interpretations where the parameter document does not give values.

This dependency-free core can be compiled and sanitizer-tested in dot. No UE5
build, Windows package run, rendering quality, sound, input-device feel or actual
GPU frame-rate result is implied by these tests. LeakSanitizer is unavailable in
this ptraced execution environment; ASan+UBSan runs use `detect_leaks=0` explicitly.
