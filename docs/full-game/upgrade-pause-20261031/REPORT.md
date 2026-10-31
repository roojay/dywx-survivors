# Upgrade selection freezes combat

The user's 2026-10-31 request replaces the earlier rule that upgrades do not pause combat. Waiting for an upgrade now freezes simulation and presentation until all queued choices are resolved.

The authoritative gate is `Simulation::IsAwaitingUpgrade()`, derived directly from progression. `Simulation::Step` checks it at entry and after each fixed substep, clearing accumulated time and queued dash input. Pickup/death-overflow rewards cannot advance the remaining substeps after producing choices. The world renderer uses the same `IsCombatFrozen()` gate for visual time. The controller clears movement/dash input while choices are pending and refuses to queue a dash during a freeze.

This automatic wait never writes `Snapshot::paused`. That flag continues to represent explicit manual/focus pause; selection commands cannot release it. Choose, reroll, seal, skip and eligible fusion remain legal during automatic waiting. The existing Codex/reading-modal pause and Rest inventory boundary remain in place. Rest checkpoint projection and restored pending choices retain their prior save format.

## Evidence

- Before the core change, the new frozen-state regression failed (`Combat(s)==before`); see `before.log`.
- After reconnecting win-dev, the current sources rebuilt and **23/23 CMake/CTest regressions passed**; see `ctest.log`.
- The new regression covers frozen enemies/projectiles/HP/effects/cooldowns/anomalies/time; consecutive choices; reroll/seal/skip; explicit pause before and after a choice appears; large-delta first-substep interruption; overflow XP; no accumulated-time catch-up or stale dash; boss phase; Rest and checkpoint resume with pending choices.
- Existing 22 tests include checkpoint replay across 54 seeded spawn cases, 3 anomaly/deferred-XP cases and 3 endless boss boundaries.
- Focus/presentation and module-layering source contracts passed; see `focus-contract.log` and `architecture-contract.log`. These are source checks, not native UI observations.
- Old tests that explicitly expected combat to continue while waiting were updated to the new rule. The economy and stress harnesses now resolve choices through normal commands before advancing. The stress report checks and prints actual simulated ticks/time. Historical results were not rewritten.

## Deliberately not performed

UE UHT/UBT, Cook/Package, new native GUI interaction and runtime visual timing remain **NOT_RUN for this fix**. The user subsequently requested implementation of five new UI concept screens, so this source change is published as the baseline for that work; the old UI is not independently repackaged. Existing packages and user saves are preserved. No new CUA authorization request was made.
