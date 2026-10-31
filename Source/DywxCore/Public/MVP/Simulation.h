#pragma once
#include "MVP/Export.h"
#include "MVP/Progression.h"
#include "MVP/Types.h"
#include "MVP/Campaign.h"
#include "MVP/Economy.h"
#include "MVP/Arena.h"
namespace dywx::mvp {
struct DYWXCORE_API RunCheckpoint;
// Provisional non-prototype cave boss tuning; authoritative for combat and UI text.
struct DYWXCORE_API CaveBossRules {
  std::array<float,2> flamePeriod{4,3}, flameDamage{18,26}, flameSummon{12,9};
  float flameWarning=1, flameRadius=2.2f, ringPeriod=7, ringRadius=6;
  std::array<float,2> rootPeriod{5.5f,4}, rootDamage{12,18}, mistSummon{11,8};
  float rootWarning=1.3f, rootRadius=3.2f, rootDuration=3, rootTick=1, rootSlow=.35f;
};
struct DYWXCORE_API OpportunityDetails {
  std::string title, description;
  unsigned experience = 0, spiritStones = 0;
};
class DYWXCORE_API Simulation {
public:
  void Start(std::uint64_t seed, HeroId hero,
             const RunConfig &config = RunConfig{});
  bool StartCampaign(const CampaignProfile &profile, GameMode mode, HeroId hero,
                     std::uint64_t seed, std::uint64_t runId,
                     CaveId cave = CaveId::ThunderMarsh, unsigned riskMask = 0,
                     const RunConfig &config = {}, const CampaignConfig &campaignConfig = {});
  bool IsCampaign() const { return fullGame_; }
  const CampaignProfile &GetCampaignProfile() const { return campaignProfile_; }
  const Campaign &GetCampaign() const { return campaign_; }
  const ArenaLayout &GetArena() const { return arena_; }
  const AnomalySystem &GetAnomalies() const { return anomalies_; }
  Economy &GetEconomy() { return economy_; }
  const Economy &GetEconomy() const { return economy_; }
  OpportunityDetails DescribeOpportunity(unsigned index) const;
  static std::string DescribeEnemy(EnemyRole role);
  static std::string DescribeMutation(unsigned index);
  static std::string DescribeElementInteractions();
  static CaveBossRules BossRules() { return {}; }
  static std::string DescribeBossMechanics(CaveId cave);
  std::string DescribeBossMechanics() const { return DescribeBossMechanics(fullGame_?arena_.cave:CaveId::ThunderMarsh); }
  bool ChooseOpportunity(unsigned index);
  bool ChooseBossReward(std::size_t index);
  bool SettleCampaign(CampaignProfile &profile);
  bool FinalizeRestExperience();
  bool PrepareCheckpoint(RunCheckpoint &out) const;
  bool RestoreCheckpoint(const RunCheckpoint &checkpoint, const CampaignProfile &profile);
  bool BuyGood(std::uint64_t token);
  bool DiscoverCurrentShop(); // UI open/refresh event only; queries remain pure.
  bool RefreshShop();
  bool BeginReforge(EntityId id, std::size_t affix);
  bool ResolveReforge(bool keepNew);
  bool OpenTreasure(bool paid = true);
  bool RevealTreasure();
  bool IdentifyRelic(EntityId id);
  bool Refine(const std::vector<EntityId> &ids, RefineReward reward);
  bool UndoRefine();
  // Gameplay commands own phase/pause validation and snapshot refresh.
  bool Equip(EntityId bagId, std::size_t slot);
  bool Unequip(std::size_t slot);
  bool SetLocked(EntityId id, bool locked);
  bool SortBag();
  bool CanFuse(SpellId result) const;
  bool Fuse(SpellId result);
  bool SealUpgrade(std::size_t index);
  bool SkipUpgrade();
  // Derived from authoritative progression, separate from manual/focus pause.
  // Choice commands remain legal while this automatic combat gate is active.
  bool IsAwaitingUpgrade() const {
    const auto& p = progression_.GetState();
    return p.pendingUpgrades > 0 && !p.offers.empty();
  }
  bool IsCombatFrozen() const {
    return state_.paused || IsAwaitingUpgrade() ||
        (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss);
  }
  void Step(float dt, const InputState &input = {});
  bool ChooseUpgrade(std::size_t index);
  bool Reroll();
  // False means combat did not start. Deferred refinement XP may be committed
  // while retaining Rest, so its newly-created upgrade choices can be resolved.
  bool BeginNextWave();
  void Pause(bool paused);
  void Restart();
  const Snapshot &GetSnapshot() const { return state_; }
  const RunConfig &GetConfig() const { return config_; }
  // Mutable access is reserved for internal adapters and deterministic fixtures.
  // Gameplay callers use the commands above, which refresh their own snapshots.
  Progression &GetProgression() { return progression_; }
  const Progression &GetProgression() const { return progression_; }
  // Refresh after internal/profile initialization, not after gameplay commands.
  void RefreshProgression();
  // Deterministic harness hooks; never bound to shipping input.
  EntityId TestSpawn(EnemyRole role, Vec2 position,
                     EnemyTier tier = EnemyTier::Normal);
  void TestDamageEnemy(EntityId id, float damage,
                       Element element = Element::None);
  void TestDamagePlayer(float damage);
  void TestCast(SpellId id, int rank = 1) { Cast({id, rank}); }
  void TestSetPlayer(Vec2 position, float hp) {
    state_.player.position = position;
    state_.player.hp = hp;
  }

private:
  bool fullGame_ = false;
  Campaign campaign_;
  CampaignProfile campaignProfile_;
  Economy economy_;
  ArenaLayout arena_;
  AnomalySystem anomalies_;
  EntityId opportunityEnemy_ = 0;
  std::uint64_t restSerial_ = 0;
  float terrainTimer_ = 0;
  bool anomalyWarningShown_ = false;
  Vec2 Constrain(Vec2 point, float radius = .45f) const;
  Vec2 Steer(Vec2 position, Vec2 direction, float distance, float radius) const;
  void ReceiveRelic(Relic relic);
  Snapshot state_;
  Progression progression_;
  RunConfig config_;
  std::uint64_t seed_ = 1, rng_ = 1, nextId_ = 1;
  HeroId hero_ = HeroId::Body;
  double accumulator_ = 0;
  bool dashLatched_ = false, dashQueued_ = false;
  float spawnAccumulator_ = 0, bossTimer_ = 0, bossSummon_ = 0, bossShield_ = 0,
        wallTimer_ = 0, chargeTimer_ = 0;
  std::array<float, static_cast<std::size_t>(SpellId::Count)> spellCooldowns_{};
  std::array<int, static_cast<std::size_t>(SpellId::Count)> casts_{};
  std::array<float, 7> procCooldowns_{};
  float speedBuff_ = 0, powerBuff_ = 0, shieldTime_ = 0, bodyProcCooldown_ = 0,
        critCooldown_ = 0, resetTimer_ = 0;
  bool lowHpUsed_ = false, lowHpActive_ = false;
  std::array<int, 6> waveSpawned_{};
  int eliteSpawned_ = 0;
  bool beastRolled_ = false, beastPending_ = false;
  int damageDepth_ = 0;
  SpellId currentSource_ = SpellId::Count;
  float Random();
  EntityId Id() { return nextId_++; }
  void Tick(float dt, const InputState &input);
  void SpawnWave(float dt);
  void UpdateEnemies(float dt);
  void UpdateSpells(float dt);
  void UpdateProjectiles(float dt);
  void UpdateEffects(float dt);
  void CollectPickups(bool all = false);
  void ResolveDeaths();
  EntityId Spawn(EnemyRole role, Vec2 position, EnemyTier tier,
                 EntityId owner = 0, bool ignorePhaseCap = false);
  Vec2 Edge();
  EnemyState *Find(EntityId id);
  EnemyState *Target(bool highest = false, float range = 100);
  Vec2 DenseTarget(float radius);
  float ScaleDamage(float damage, Element element, Form form) const;
  void Hit(EnemyState &enemy, float damage, Element element, Form form,
           bool primary = true, float burn = 0, float burnDuration = 0,
           float stun = 0);
  void Area(Vec2 at, float radius, float damage, Element element, Form form,
            bool primary = true, float burn = 0, float burnDuration = 0,
            float stun = 0);
  void Chain(EntityId start, int count, float damage, float scale, float radius,
             Form sourceForm, bool highest = false);
  void Hurt(float damage, DamageKind kind = DamageKind::Unknown,
            EntityId source = 0, Vec2 position = {},
            EffectKind effectKind = EffectKind::Circle);
  bool AddEffect(EffectState effect);
  void AddProjectile(ProjectileState projectile);
  void Cast(SpellState spell);
  void EndWave();
  void StartBoss();
  void BossPhaseTwo(EnemyState &boss);
  void UpdateCaveBoss(EnemyState &boss, float dt);
  void Sync();
};
} // namespace dywx::mvp
