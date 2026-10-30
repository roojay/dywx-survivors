#pragma once
#include "MVP/Types.h"
#include <array>
#include <cstdint>
#include <vector>
namespace dywx::mvp {
enum class GameMode { Quick, Traditional, Endless };
enum class Feature { Inventory, ElementResonance, Fusion, TriggerAffix, Luck, Reroll,
  FormResonance, Economy, SealSkip, Anomalies, Reforge, Chest, Trials, Endless };
enum class OpportunityKind { Elite, Event, Cave, Chest };
enum class CampaignPhase { Inactive, Wave, Intermission, Boss, OpportunityCombat, Won, Dead };
enum class LegacyUnlock { HeroSword, HeroBeast, RealmSecond, RealmThird, SpellPool,
  RelicPool, ExtraReroll, FreeCaveRefresh, Count };
enum class PermanentStat { Health, Armor, Regen, Power, Cooldown, Area, Critical,
  Speed, Pickup, Dash, Count }; // Luck deliberately cannot be purchased.
struct HeroRecord {
  int hero = 0, bestWave = 0;
  std::uint64_t kills = 0, runs = 0, wins = 0, mastery = 0;
};
struct EndlessRecord {
  std::uint64_t runId = 0;
  int hero = 0, wave = 0;
  std::uint64_t kills = 0;
};
struct CampaignProfile {
  std::uint64_t daoYun = 0;
  bool anyWin = false, traditionalWin = false;
  std::array<bool, 6> seenAnomalies{}; // account discovery only, no combat power
  std::array<bool, 3> seenBosses{}; // distinct cave bosses; never infer all from generic Boss role
  unsigned unlockedRisk = 1;
  std::array<bool, static_cast<unsigned>(LegacyUnlock::Count)> unlocks{};
  std::array<unsigned, static_cast<unsigned>(PermanentStat::Count)> bonusRanks{};
  std::vector<HeroRecord> heroes;
  std::vector<EndlessRecord> endlessRanking; // local top 20; wave desc, kills desc, runId asc, hero asc
  std::uint64_t nextRunId = 1;
  std::array<std::uint64_t, 3> settledThrough{}; // per-hero monotonic high watermarks
};
struct CampaignConfig {
  bool quickSixWaves = false; // explicit paired-config A/B experiment; default remains four
  // Provisional tuning: 10*198s + ~120s boss = 35min (excluding menus).
  float traditionalWaveSeconds = 198, endlessWaveSeconds = 180;
  float riskRewardPerPoint = .15f;
  unsigned endlessBossReward = 5, killsPerDaoYun = 0;
  unsigned legacyUnlockCost = 20, permanentRankCost = 10;
  unsigned eventStones = 20, eventXp = 15;
};
struct WaveSchedule {
  bool valid = false, bossAfter = false;
  float seconds = 0, healthScale = 1, damageScale = 1;
  int enemyCap = 30, eliteCount = 1;
  std::array<int, 6> roleCounts{};
};
struct TrialEffects {
  unsigned risk = 0;
  float population = 1, speed = 1, elite = 1, hazard = 1, highRarityWeight = 1;
};
struct CampaignRunState {
  std::uint64_t runId = 0, rng = 1, kills = 0;
  GameMode mode = GameMode::Quick;
  int hero = 0, wave = 1, bosses = 0;
  unsigned riskMask = 0, tier = 1;
  CampaignPhase phase = CampaignPhase::Inactive;
  std::array<OpportunityKind, 3> offers{};
  unsigned offerCount = 0;
  int selected = -1, caveAbsent = 0, chestAbsent = 0;
  bool opportunityResolved = false, settled = false;
};
struct OpportunityResolution {
  bool accepted = false;
  OpportunityKind kind = OpportunityKind::Event;
  unsigned spiritStones = 0, xp = 0;
  bool eliteEncounter = false, openCave = false, openChest = false;
};
std::uint64_t ReserveRunId(CampaignProfile &profile); // persist reservation before starting
unsigned FeatureTier(const CampaignProfile &profile);
bool HasFeature(unsigned tier, Feature feature);
bool HasFeature(const CampaignProfile &profile, Feature feature);
bool IsHeroUnlocked(const CampaignProfile &profile, int hero);
bool IsRealmUnlocked(const CampaignProfile &profile, int realm);
WaveSchedule ScheduleWave(GameMode mode, int wave, const CampaignConfig &config = {});
TrialEffects GetTrialEffects(unsigned mask, const CampaignConfig &config = {});
bool PurchaseUnlock(CampaignProfile &profile, LegacyUnlock unlock, const CampaignConfig &config = {});
bool PurchaseBonus(CampaignProfile &profile, PermanentStat stat, const CampaignConfig &config = {});
float PermanentBonus(const CampaignProfile &profile, PermanentStat stat);
void ApplyPermanentBonuses(const CampaignProfile &profile, Stats &stats);
class Campaign {
public:
  explicit Campaign(const CampaignConfig &config = {});
  bool Start(const CampaignProfile &profile, GameMode mode, int hero,
             std::uint64_t seed, std::uint64_t runId, unsigned riskMask = 0);
  bool CompleteWave();
  bool BeginNextWave(); // transitions final rest to boss; endless bosses each 5 waves
  bool CompleteBoss();
  OpportunityResolution ChooseOpportunity(unsigned index);
  bool CompleteOpportunity(); // only after the chosen elite actually dies
  bool RecordKills(std::uint64_t count);
  bool Die();
  std::uint64_t Reward() const;
  bool Settle(CampaignProfile &profile); // durable profile run-id ledger prevents duplicates
  const CampaignRunState &GetState() const { return state_; }
  const CampaignConfig &GetConfig() const { return config_; }
  bool Restore(const CampaignRunState &state); // validation, no RNG consumption
private:
  CampaignConfig config_;
  CampaignRunState state_;
  std::uint64_t Random();
  void Offer();
};
} // namespace dywx::mvp
