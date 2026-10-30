# MVP progression / content contract

This module uses `gameplay-design-v0.1.md` and `mvp-prototype-config-v0.1.md`, not the earlier three-spell UI prototype. It contains no UE includes. Numeric spell definitions and ranks are in `ContentCatalog.cpp`; run progression, equipment and deterministic rolls are in `Progression.cpp`.

## Runtime boundary

- `Progression::Reset(seed, hero, formResonance)` starts a fresh run. It owns an explicit deterministic `std::mt19937` progression stream; combat owns a separate deterministic seed stream. Neither uses wall time, `random_device`, hidden global random state, or paid services. `Unit()` uses explicit integer-to-float mapping rather than implementation-dependent `uniform_real_distribution`.
- `AddXp` applies current XP gain once, queues upgrades, and never pauses combat. The next threshold is 23, then 31, 39, etc. Offers are distinct legal options; categories use 30:45:25, with luck scaling new spells/passives, rank-five partner ×2, and an affinity-guaranteed first draw when possible. If fewer than three legal choices remain, show only those; if none remain, consume exhausted queued choices rather than deadlock.
- Starter exclusivity follows the design document: FlamePalm is Body-only and FlameSword is Sword-only. Each hero has nine eligible base spells; the overall catalog has ten. Fusion recipes are also hero-restricted.
- A hero's dedicated passive is always present and occupies one of the four passive slots. Generic passives are one-time unlocks (no numeric passive rank table exists). Two rerolls are per run. Fusions consume their two rank-five constituents, preserve both resonance counts, and free one slot. Consumed ingredients cannot be reacquired, preventing repeated fusion or resonance duplication. Fusion is available during an upgrade selection or rest, and does not consume an upgrade.
- Shape-axis resonance is configurable and defaults on, as recommended in config §12. Body armor/damage bonuses are already folded into derived stats. Combat must apply the remaining body/sword/array behavior from `BuildState.resonance` only when its config switch is enabled.
- `Stats.triggers[0..6]`, `mysteries[0..5]`, `uniques[0..1]` represent T1–T7, M1–M6, U1–U2. Duplicate equipment copies do not create multiple procs or separate cooldowns. Combat owns trigger counters/cooldowns and recursion bounds. Element/form bonus arrays are additive damage bonuses. Burn/chain/XP multipliers include passives and applicable elemental resonance.
- Affix numbers are stored as proportions (0.08 = 8%), fixed grid values, or fixed seconds. Movement bonuses use base 5 grid/sec. Stats caps are applied after all bonuses. `EffectiveLuck` is the only diminishing-returns conversion; raw luck is preserved for display.

## Inventory and reversible refinement

- Pickups autoequip until six slots, except conflicting M1/M2 relics which enter the bag even with free slots. Full slots always route to bag. Nothing auto-refines. Duplicate item IDs cannot be granted twice.
- Equip/swap, unequip, lock, sort, refine and undo require `BeginRest`; readonly filtering and stat comparison are always available. Locks protect refinement, not deliberate swaps. Refinement only targets bag items; equipped items must first be unequipped.
- Preview deduplicates IDs and excludes nonexistent/locked items. Confirm recalculates the preview, so stale selection cannot refine a newly locked item. Undo is LIFO and returns the exact original relic identities/affix rolls to the bag.
- **Implementation resolution:** refinement XP is visibly pending during rest, with fixed 4/12/40/100 values. `EndRest` commits it exactly once with no XP gain bonus, clears the undo ledger, and may queue upgrade choices. This delays spendable XP until rest exit to make undo safe without rolling back a player's upgrade choices. Re-entering rest does not reopen a previous undo ledger.
- `PreviewEquip` computes full derived stats, including passives, resonance and caps, for side-by-side deltas. `RelicFilter` supports rarity, aspect, affix kind and unlocked-only. Sorting is stable by rarity descending, catalog ID, item ID.

## Drops, themes and persistence

- All 15 themes have their mandatory affixes, with numeric values uniformly rolled within the quality ranges. The mandatory common/spirit/immortal lists already fill their maximum 2/3/4 slots; therefore those themes have no spare random affix-kind slot. Immortal trigger identity is random. Dao themes roll one property, one aspect, one trigger and a distinct extra property/aspect/resonance, plus their fixed mystery and unique (five ordinary affixes + one unique). This respects mandatory theme lists over the more general 1–2/2–3 slot ranges.
- Special resonance affix contributes once total across equipment (first equipped occurrence); U2 is also deduplicated and grants fire +1 and thunder +1. No random mystery is added on top of a theme's fixed mystery. M1/M2 exclusion is enforced for automatic and manual equipment changes.
- Normal/variant drops are 0.5%/2.5%; elite and beast guarantee Spirit+. Quality weights match §8.9 and luck modifies Spirit+ weight only, not drop chance. Boss offers contain three distinct Immortal/Dao catalog items, exclude pity, and require an explicit choice.
- `OnWaveEnded` updates the consecutive wave counter once per wave; a random Immortal resets it. After 20 unsuccessful completed waves, the next successful random drop is Immortal. Dao never resets or satisfies Immortal pity. Call `SetPityWaves` after reset to import a persisted value; `PityWaves` exports it. **There is no disk persistence in this subsystem**; the host save layer imports/exports the counter. `MergeDiscoveries(previous)` ORs only seenSpells/seenRelics/seenFusions after reset; it never copies XP, inventory, stats, build, or pity.
- Spirit beast appearance uses 40% × the effective-luck weight multiplier (at most 64%); simulation calls it once per run. It does not use drop pity.

## Known table resolutions

- FlameBody/FlameArray say “灼烧附带/阵内灼烧” without a separate numeric damage/duration; use FlamePalm's 3/sec for 3 sec. FusionSwords uses FlameSword's 2/sec for 2 sec and ThunderSword's 50% chain ratio. These are explicit minimal baselines, not claimed source numbers.
- FlameSword/ThunderSword targeting reach is 12/14 grid units because the numeric table specifies piercing/homing but no maximum travel distance. Body ThunderFist uses a forward 120° cone to preserve the stated melee-facing control. Shape-index/UI values remain Fire, Thunder, Body, Sword, Array.
- Boss quality choice weights are not specified. The six eligible catalog entries are sampled without replacement (4 Immortal, 2 Dao), making the three offers distinct; this is independent of the random-drop quality distribution.
- Currency, shops, sealing/skipping, reforging, anomalies, endless mode, and carry-out rewards are absent by scope. `Relic.carryOut` is reserved and false.

## Validation

Run `bash scripts/test-mvp-progression.sh`. The suite covers threshold arithmetic, deterministic offers and rolls, affinity, rerolls, all spell ranks, restricted fusions and inherited counts, slot limits, duplicates, quality/theme counts, rest gates, locks/refine preview/undo, M1/M2, shared effects, caps, pity, boss reward uniqueness and a 100,000-kill drop smoke check. This is a CPU data/subsystem test, not UE build or executable validation.
