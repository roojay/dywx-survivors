#pragma once
#include "MVP/Export.h"
#include "MVP/ContentCatalog.h"
#include <optional>
#include <random>
namespace dywx::mvp {
struct DYWXCORE_API RefinePreview {
  std::vector<EntityId> ids;
  int xp = 0;
};
struct DYWXCORE_API RelicFilter {
  int rarity = -1;
  int aspect = -1;
  int affixKind = -1;
  bool unlockedOnly = false;
};
class DYWXCORE_API Progression {
public:
  void ConfigureFullGame(int layer, const Stats& permanentBonuses = Stats{}, int extraRerolls = 0);
  void SetSystemLayer(int layer);
  void SetContentUnlocks(bool spellPool, bool relicPool);
  bool SealOffer(std::size_t index);
  bool SkipUpgrade();
  int TakeSkipStones();
  bool FullGameEnabled() const { return fullGame_; }
  void Reset(std::uint32_t seed, HeroId hero, bool formResonance = true);
  const ProgressionState &GetState() const { return state_; }
  // Imports discovery flags only; never run power, inventory, XP or pity.
  void MergeDiscoveries(const ProgressionState &previous);
  // An enemy entering the active arena counts as encountered, even off camera.
  // Discovery metadata only: no RNG draws or changes to combat state.
  void RecordEncounter(EnemyRole role, int mutations);
  void
  AddXp(float xp); // Kill/pickup XP receives xpMultiplier. Refinement does not.
  bool ChooseUpgrade(std::size_t index);
  bool Reroll();
  bool CanFuse(SpellId result) const;
  bool Fuse(SpellId result);
  void GrantRelic(Relic relic);
  std::optional<Relic> OnKill(EnemyTier tier, bool spiritBeast = false);
  Relic
  GenerateRelic(Rarity rarity); // Explicit rarity, same seeded RNG stream.
  Relic GenerateRelicFromCatalog(int catalogId);
  std::vector<Relic> BossRewards(); // Three distinct catalog items,
                                    // Immortal/Dao only, no pity impact.
  void GenerateBossRewards();
  void GenerateBossRewards(std::uint64_t bossOrdinal);
  void SetDropRiskWeight(float weight);
  std::array<float,4> DropRarityProbabilities(bool guaranteedSpirit = false) const;
  std::string DescribeDropOdds(bool guaranteedSpirit = false) const;
  void SetRelicPool(const std::vector<int>& preferredCatalogIds);
  bool ChooseBossReward(std::size_t index);
  bool RollSpiritBeast();
  void OnWaveEnded();
  int PityWaves() const { return pityWaves_; }
  void
  SetPityWaves(int waves); // External save integration, no hidden persistence.
  void BeginRest();
  void EndRest();
  bool IsResting() const { return resting_; }
  bool Equip(EntityId bagId, std::size_t slot);
  bool Unequip(std::size_t slot);
  bool SetLocked(EntityId id, bool locked);
  void SortBag();
  std::vector<EntityId> FilterBag(const RelicFilter &filter = {}) const;
  RefinePreview PreviewRefine(const std::vector<EntityId> &ids) const;
  bool Refine(const std::vector<EntityId> &ids);
  bool UndoRefine();
  int PendingRefineXp() const { return pendingRefineXp_; }
  Stats PreviewEquip(EntityId bagId, std::size_t slot) const;
  bool HasTrigger(int code) const;
  bool HasMystery(int code) const;
  bool HasUnique(int code) const;
  static Stats ClampStats(Stats s);

private:
  friend class SaveCodec;
  bool fullGame_ = false, spellPoolUnlocked_ = true, relicPoolUnlocked_ = true;
  Stats permanentBase_{};
  float dropRiskWeight_ = 1;
  std::vector<int> preferredRelics_;
  std::uint64_t lastBossOrdinal_ = 0;
  friend class Economy; // Atomic inventory transactions and stat recomputation.
  ProgressionState state_;
  std::mt19937 rng_{1};
  EntityId nextRelic_ = 1;
  bool formResonance_ = true, resting_ = false, immortalThisWave_ = false,
       bossRewardsGenerated_ = false;
  int pityWaves_ = 0, pendingRefineXp_ = 0;
  std::array<bool, kSpellCount> consumed_{};
  struct Undo {
    std::vector<Relic> relics;
    int xp = 0;
  };
  std::vector<Undo> undo_;
  bool LegalRelic(int catalogId) const;
  float Unit();
  int Index(int size);
  void AddRawXp(float xp);
  void MakeOffers();
  void Recompute();
  BuildState Derive(const std::vector<Relic> &equipment) const;
  std::vector<UpgradeOffer> LegalOffers() const;
  bool Affinity(const UpgradeOffer &offer) const;
  bool Compatible(const Relic &relic,
                  std::optional<std::size_t> replace = {}) const;
  bool HasSpell(SpellId id) const;
  bool HasPassive(PassiveId id) const;
  Rarity RollRarity(bool guaranteedSpirit);
};
} // namespace dywx::mvp
