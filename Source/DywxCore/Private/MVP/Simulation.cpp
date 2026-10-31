#include "MVP/Simulation.h"
#include "MVP/ContentCatalog.h"
#include "MVP/SaveCodec.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
namespace dywx::mvp {
namespace {
constexpr float Pi = 3.14159265359f;
constexpr std::array<float,8> EnemyHp{18,14,90,70,120,160,14000,80};
constexpr std::array<float,8> EnemySpeed{3.2f,2.6f,2.8f,2,1.6f,1.2f,1.5f,6};
constexpr std::array<float,8> EnemyDamage{6,25,30,14,0,0,35,0};
float Len2(Vec2 a) { return a.x * a.x + a.y * a.y; }
float Len(Vec2 a) { return std::sqrt(Len2(a)); }
Vec2 Unit(Vec2 a) {
  float l = Len(a);
  return l > .0001f ? a * (1 / l) : Vec2{1, 0};
}
Vec2 Clamp(Vec2 a, float r = .45f) {
  a = {std::clamp(a.x, -48.f + r, 48.f - r),
       std::clamp(a.y, -27.f + r, 27.f - r)};
  for (const auto &obstacle : GetArenaObstacles()) {
    Vec2 delta = a - obstacle.position;
    float distance = Len(delta), minimum = r + obstacle.radius;
    if (distance < minimum)
      a = obstacle.position + Unit(delta) * minimum;
  }
  return a;
}
float SegmentDistance(Vec2 p, Vec2 a, Vec2 b) {
  Vec2 d = b - a;
  float q = Len2(d);
  float t =
      q > 0 ? std::clamp(((p.x - a.x) * d.x + (p.y - a.y) * d.y) / q, 0.f, 1.f)
            : 0;
  return Len(p - (a + d * t));
}
void Down(float &v, float dt) { v = std::max(0.f, v - dt); }
int Index(SpellId s) { return static_cast<int>(s); }
} // namespace
float Simulation::Random() {
  rng_ ^= rng_ >> 12;
  rng_ ^= rng_ << 25;
  rng_ ^= rng_ >> 27;
  return float((rng_ * 2685821657736338717ULL) >> 40) / 16777216.f;
}
void Simulation::Start(std::uint64_t seed, HeroId hero,
                       const RunConfig &config) {
  fullGame_ = false;
  opportunityEnemy_ = 0;
  restSerial_ = 0;
  terrainTimer_ = 0;
  anomalyWarningShown_ = false;
  const auto gen = state_.generation + 1;
  state_ = Snapshot{};
  state_.generation = gen;
  seed_ = seed;
  rng_ = seed ? seed : 1;
  nextId_ = 1;
  hero_ = hero;
  config_ = config;
  config_.fixedStep = std::isfinite(config_.fixedStep)
      ? std::clamp(config_.fixedStep, 1.f / 240, 1.f / 20)
      : RunConfig{}.fixedStep;
  config_.maxEnemies = std::clamp<std::size_t>(config_.maxEnemies, 1, 512);
  config_.maxProjectiles =
      std::clamp<std::size_t>(config_.maxProjectiles, 1, 2048);
  config_.maxEffects = std::clamp<std::size_t>(config_.maxEffects, 1, 2048);
  config_.maxPickups = std::clamp<std::size_t>(config_.maxPickups, 1, 4096);
  for (auto &duration : config_.waveDurations)
    duration = std::max(.1f, duration);
  state_.enemies.reserve(config_.maxEnemies);
  state_.projectiles.reserve(config_.maxProjectiles);
  state_.effects.reserve(config_.maxEffects);
  state_.pickups.reserve(config_.maxPickups);
  accumulator_ = 0;
  dashLatched_ = dashQueued_ = false;
  spawnAccumulator_ = bossTimer_ = bossSummon_ = bossShield_ = wallTimer_ =
      chargeTimer_ = 0;
  spellCooldowns_.fill(0);
  casts_.fill(0);
  procCooldowns_.fill(0);
  speedBuff_ = powerBuff_ = shieldTime_ = bodyProcCooldown_ = critCooldown_ =
      resetTimer_ = 0;
  lowHpUsed_ = lowHpActive_ = false;
  eliteSpawned_ = 0;
  waveSpawned_.fill(0);
  beastRolled_ = beastPending_ = false;
  damageDepth_ = 0;
  progression_.Reset(seed ^ 0x9e3779b97f4a7c15ULL, hero, config_.formResonance);
  state_.wave = 1;
  state_.waveCount = config_.sixWaves ? 6 : 4;
  state_.waveDuration = config_.waveDurations[0];
  state_.phase = RunPhase::Wave;
  Sync();
  state_.player.hp = state_.progression.build.stats.maxHp;
}
bool Simulation::StartCampaign(const CampaignProfile &profile, GameMode mode, HeroId hero,
    std::uint64_t seed, std::uint64_t runId, CaveId cave, unsigned riskMask,
    const RunConfig &config, const CampaignConfig &campaignConfig) {
  seed=seed?seed:1;
  CampaignConfig cc=campaignConfig;cc.quickSixWaves=mode==GameMode::Quick && config.sixWaves;
  Campaign next(cc);
  if (!IsRealmUnlocked(profile, int(cave)) ||
      !next.Start(profile, mode, int(hero), seed, runId, riskMask)) return false;
  RunConfig c = config;
  c.sixWaves = mode==GameMode::Quick && config.sixWaves;
  c.formResonance = HasFeature(profile, Feature::FormResonance);
  Start(seed, hero, c);
  fullGame_ = true;
  campaign_ = next;
  campaignProfile_ = profile;
  Stats baseline; ApplyPermanentBonuses(profile, baseline);
  progression_.ConfigureFullGame(int(FeatureTier(profile)), baseline,
      profile.unlocks[unsigned(LegacyUnlock::ExtraReroll)] ? 1 : 0);
  progression_.SetContentUnlocks(profile.unlocks[unsigned(LegacyUnlock::SpellPool)], profile.unlocks[unsigned(LegacyUnlock::RelicPool)]);
  arena_ = GenerateArena(seed, cave);
  progression_.SetDropRiskWeight(GetTrialEffects(riskMask, campaignConfig).highRarityWeight);
  progression_.SetRelicPool(RealmRelicPool(int(cave)));
  AnomalyConfig ac;
  const auto trial = GetTrialEffects(riskMask, campaignConfig);
  ac.lightningDamage *= trial.hazard; ac.fissureDamage *= trial.hazard;
  anomalies_.Reset(seed, arena_, HasFeature(profile, Feature::Anomalies), ac);
  EconomyConfig ec;ec.rarityWeight=trial.highRarityWeight;
  economy_.Reset(unsigned(seed), mode == GameMode::Quick,
      HasFeature(profile, Feature::Economy), HasFeature(profile, Feature::Reforge), ec);
  state_.waveCount = mode == GameMode::Quick ? (c.sixWaves?6:4) : mode == GameMode::Traditional ? 10 : 0;
  state_.waveDuration = ScheduleWave(mode, 1, cc).seconds;
  if (mode == GameMode::Quick) state_.waveDuration = config_.waveDurations[0];
  Sync(); state_.player.hp = state_.progression.build.stats.maxHp;
  return true;
}
void Simulation::Restart() {
  if (fullGame_) {
    // A new run needs a new durable id supplied by the caller, not a replayed settlement id.
    return;
  }
  Start(seed_, hero_, config_);
}
Vec2 Simulation::Constrain(Vec2 p, float radius) const {
  return fullGame_ ? arena_.Constrain(p, radius) : Clamp(p, radius);
}
Vec2 Simulation::Steer(Vec2 position, Vec2 direction, float distance, float radius) const {
  if (!fullGame_) return Clamp(position + direction * distance, radius);
  Vec2 next = position + direction * distance;
  if (arena_.Walkable(next, radius)) return next;
  // Deterministic wall sliding, testing both tangents against the same layout.
  for (float angle : {.5f, -.5f, 1.f, -1.f, 1.57f, -1.57f}) {
    Vec2 rotated{direction.x*std::cos(angle)-direction.y*std::sin(angle),
                 direction.x*std::sin(angle)+direction.y*std::cos(angle)};
    next = position + rotated * distance;
    if (arena_.Walkable(next, radius)) return next;
  }
  return arena_.Constrain(position, radius);
}
void Simulation::ReceiveRelic(Relic relic) {
  if (fullGame_) economy_.ReceiveRelic(progression_, std::move(relic));
  else progression_.GrantRelic(std::move(relic));
}
std::string Simulation::DescribeEnemy(EnemyRole role) {
  unsigned index=unsigned(role);if(index>=EnemyHp.size())return {};
  static const char* rules[]={
    "群行追击，近身每0.8秒攻击一次。",
    "接近2.5格后预警0.8秒，自爆半径2.5格；雷象可提前引爆并伤及周围妖魔。",
    "12格内预警1秒后沿直线冲锋，速度9格/秒、持续1.4秒，冷却4秒。",
    "保持5至8格距离；预警0.6秒后发射灵火，速度7格/秒、冷却2秒。",
    "每5秒蓄势0.6秒，为8格内最多3只同伴各施40点护盾；完整模式火象可破除。",
    "每4秒蓄势0.6秒召3只群行妖魔，自身随从上限12只。",
    "分两变；具体洞天机制、每次预警与数值见各洞主条目。",
    "沿本洞天外围安全路线逃跑；击败必得灵品及以上法宝。"
  };
  std::ostringstream text;text<<"基础气血 "<<EnemyHp[index]<<"，身法 "<<EnemySpeed[index]<<"格/秒，基础伤害 "<<EnemyDamage[index]<<"。"<<rules[index]
      <<" 数值再乘当前波数与劫数系数；大妖气血×8、伤害×1.5，并有1至2条异变。";
  return text.str();
}
std::string Simulation::DescribeElementInteractions() {
  return "雷击磷火：提前引爆，2.5格范围内对周围妖魔造成25点基础伤害。火击灵盾：完整模式直接破除护盾，不能破除灵甲。水击灼烧妖魔：本次伤害提高25%；水象术法触及地面火域时可熄灭火域。任一道象均可伤害所有妖魔，同元素不免疫。";
}
std::string Simulation::DescribeMutation(unsigned index) {
  static const char* rules[]={
    "狂暴：身法提高20%；冲锋冷却4→2.8秒，远程攻击冷却2→1.4秒。",
    "灵甲：200点灵甲；灵甲存在时伤害先降低30%，再由灵甲吸收，耗尽后失效。",
    "分裂：陨落时分出2只同职责异种，各有本体最大气血的40%；分身不继承异变。",
    "环绕雷：3枚雷珠沿1.7格轨道环绕，接触0.65格内造成6伤害，共享0.8秒命中冷却。"
  };
  return index<4?rules[index]:"";
}
std::string Simulation::DescribeBossMechanics(CaveId cave) {
  std::ostringstream text;
  const auto r=BossRules();
  if(cave==CaveId::ThunderMarsh)
    return "流波夔君：一变每6秒三道引雷（预警1秒，35伤害），每5秒召20凡妖；二变每8秒火墙（预警1.5秒，每0.25秒5伤害），每10秒蓄力冲撞（预警1.2秒，45伤害），每8秒召4远程与6冲锋妖魔。";
  if(cave==CaveId::EmberMountain) {
    text<<"赤山离火君（初版数值）：一/二变每"<<r.flamePeriod[0]<<"/"<<r.flamePeriod[1]
        <<"秒在面前铺三道火浪，预警"<<r.flameWarning<<"秒，半径"<<r.flameRadius
        <<"，伤害"<<r.flameDamage[0]<<"/"<<r.flameDamage[1]<<"；二变每"<<r.ringPeriod
        <<"秒铺八处环火。火域可被水象熄灭。每"<<r.flameSummon[0]<<"/"<<r.flameSummon[1]
        <<"秒召2自爆妖魔/2远程与2冲锋妖魔。";
  } else {
    text<<"青岚蜃君（初版数值）：一/二变每"<<r.rootPeriod[0]<<"/"<<r.rootPeriod[1]
        <<"秒布1/2处根缚区域，预警"<<r.rootWarning<<"秒，半径"<<r.rootRadius<<"，持续"
        <<r.rootDuration<<"秒，每"<<r.rootTick<<"秒伤害"<<r.rootDamage[0]<<"/"<<r.rootDamage[1]
        <<"；踏入后身法暂降"<<r.rootSlow*100<<"%，离开即恢复，遁术距离不变。每"
        <<r.mistSummon[0]<<"/"<<r.mistSummon[1]<<"秒召2群行与1护持/1护持与1召唤妖魔。";
  }
  return text.str();
}
OpportunityDetails Simulation::DescribeOpportunity(unsigned index) const {
  OpportunityDetails result;
  if(!fullGame_ || index>=campaign_.GetState().offerCount)return result;
  const auto &cave=CaveCatalog()[int(arena_.cave)];
  const auto kind=campaign_.GetState().offers[index];
  if(kind==OpportunityKind::Event) {
    // No RNG is consumed by rendering or re-opening the choice panel.
    std::uint64_t key=seed_ ^ (std::uint64_t(state_.wave)*0x9e3779b97f4a7c15ULL) ^ index;
    key^=key>>30;key*=0xbf58476d1ce4e5b9ULL;key^=key>>27;
    unsigned fragment=unsigned(key%3);
    static const char* fragments[3][3]={
      {"石上雷纹每逢潮落便依次亮起。残句曰：‘雷非无常，循脉而行。’你沿旧刻静观，悟得一缕周天。",
       "旋龟绕过雷池，始终踏在相隔等距的旧石上。它回望一瞬，留下一枚温热的灵脉结晶。",
       "池中玉片随雷声共振，池外却寂然无声。守池残铭曰：‘同声者应，同道者鸣。’"},
      {"赤羽落处，灰烬排成细密同心纹。石缝旧句曰：‘离火聚则生焰，散则成暖。’",
       "泉边一缕残念问：‘山火为何不越此线？’你循冷暖交界而行，见旧阵仍在缓缓吐纳。",
       "匣面朱纹明灭如息，温热只沿刻线流转。残铭曰：‘守其脉，火自归途。’"},
      {"古木年轮藏着反复出现的符号，月明时微亮。残篇曰：‘万木异形，生机同源。’",
       "白鹿在浓雾中只循一线清风前行。旧碑记着：‘不逐幻影，但辨来路。’",
       "雾里数座山峰同时倒影于露珠。远处传来残音：‘所见非尽虚，所知亦未全。’"}
    };
    result.title=std::string(cave.opportunities[fragment]);
    result.description=fragments[int(arena_.cave)][fragment];
    result.experience=campaign_.GetConfig().eventXp;
    result.spiritStones=HasFeature(campaign_.GetState().tier,Feature::Economy)?campaign_.GetConfig().eventStones:0;
    result.description+="\n所得修为 +"+std::to_string(result.experience);
    if(result.spiritStones)result.description+="，灵石 +"+std::to_string(result.spiritStones);
    result.description+="（初版数值）。";
  } else if(kind==OpportunityKind::Elite) {
    result.title=std::string(cave.enemies[1])+" · 大妖";
    result.description="迎战此地异变大妖；击败本体与分裂余妖后，必得灵品及以上法宝。迎战前可先整理法宝。";
  } else if(kind==OpportunityKind::Cave) {
    result.title=std::string(cave.name)+" · 遗留洞府";
    result.description="旧摊位循灵脉明灭。可祭炼仙品以上的未知灵纹；已开放坊市时可买卖整理，已开放重铸时可重铸普通灵纹。";
  } else {
    result.title=std::string(cave.name)+" · 封灵宝匣";
    result.description="此机缘免费开启一次宝匣，亲手揭示所得法宝。仙品有保底，道品无保底；额外开匣仍须支付灵石。";
  }
  return result;
}
bool Simulation::ChooseOpportunity(unsigned index) {
  if (!fullGame_ || state_.paused || state_.phase != RunPhase::Rest) return false;
  const auto result = campaign_.ChooseOpportunity(index);
  if (!result.accepted) return false;
  progression_.AddXp(float(result.xp)); economy_.CreditStones(int(result.spiritStones));
  if (result.openCave) economy_.EnterCave(progression_,
      campaignProfile_.unlocks[unsigned(LegacyUnlock::FreeCaveRefresh)]);
  if (result.openChest) economy_.OpenTreasure(progression_, false, true);
  // An elite choice reserves the encounter; inventory remains editable until
  // the player's explicit BeginNextWave command starts that fight.
  Sync(); return true;
}
bool Simulation::ChooseBossReward(std::size_t index) {
  if (state_.paused || (state_.phase != RunPhase::Won && state_.phase != RunPhase::Rest)) return false;
  bool ok = fullGame_ ? economy_.ChooseBossReward(progression_, index) : progression_.ChooseBossReward(index); Sync(); return ok;
}
bool Simulation::SettleCampaign(CampaignProfile &profile) {
  return fullGame_ && campaign_.Settle(profile);
}
bool Simulation::FinalizeRestExperience() {
  if(!fullGame_ || state_.phase!=RunPhase::Rest) return false;
  bool ok=economy_.CommitRestExperience(progression_);Sync();return ok;
}
bool Simulation::PrepareCheckpoint(RunCheckpoint &out) const {
  if(!fullGame_ || state_.phase!=RunPhase::Rest ||
      campaign_.GetState().mode==GameMode::Quick || !state_.progression.bossRewards.empty() ||
      economy_.HasTreasureToReveal() || economy_.PendingReforge()) return false;
  Campaign next=campaign_;
  if(!next.BeginNextWave())return false;
  RunCheckpoint checkpoint;
  checkpoint.progression=progression_;checkpoint.economy=economy_;
  checkpoint.economy.EndRest(checkpoint.progression);checkpoint.progression.EndRest();
  checkpoint.campaign=next.GetState();checkpoint.campaignConfig=campaign_.GetConfig();
  checkpoint.runConfig=config_;checkpoint.seed=seed_;checkpoint.simulationRng=rng_;
  checkpoint.nextEntityId=nextId_;checkpoint.restSerial=restSerial_;checkpoint.realm=int(arena_.cave);
  checkpoint.player=state_.player;checkpoint.statistics=state_.statistics;
  checkpoint.arena=arena_;checkpoint.anomalies=anomalies_;
  checkpoint.spellCooldowns=spellCooldowns_;checkpoint.casts=casts_;
  checkpoint.combat.procCooldowns=procCooldowns_;
  checkpoint.combat.speedBuff=speedBuff_;checkpoint.combat.powerBuff=powerBuff_;
  checkpoint.combat.shieldTime=shieldTime_;checkpoint.combat.bodyProcCooldown=bodyProcCooldown_;
  checkpoint.combat.critCooldown=critCooldown_;checkpoint.combat.resetTimer=resetTimer_;
  checkpoint.combat.terrainTimer=terrainTimer_;
  checkpoint.combat.lowHpUsed=checkpoint.combat.lowHpActive=false;
  if(!SaveCodec::ValidateCheckpoint(checkpoint))return false;
  out=std::move(checkpoint);return true;
}
bool Simulation::RestoreCheckpoint(const RunCheckpoint &checkpoint, const CampaignProfile &profile) {
  if(!SaveCodec::ValidateCheckpoint(checkpoint))return false;
  const auto &run=checkpoint.campaign;
  if(run.hero<0 || run.hero>=3 || run.runId<=profile.settledThrough[unsigned(run.hero)] ||
      !IsHeroUnlocked(profile,run.hero) || !IsRealmUnlocked(profile,checkpoint.realm) ||
      run.tier>FeatureTier(profile))return false;
  Campaign restored(checkpoint.campaignConfig);
  if(!restored.Restore(run))return false;
  Start(checkpoint.seed,HeroId(run.hero),checkpoint.runConfig);
  fullGame_=true;campaign_=restored;campaignProfile_=profile;
  progression_=checkpoint.progression;economy_=checkpoint.economy;
  seed_=checkpoint.seed;rng_=checkpoint.simulationRng;nextId_=checkpoint.nextEntityId;
  restSerial_=checkpoint.restSerial;arena_=checkpoint.arena;anomalies_=checkpoint.anomalies;
  state_.player=checkpoint.player;state_.statistics=checkpoint.statistics;
  spellCooldowns_=checkpoint.spellCooldowns;casts_=checkpoint.casts;
  procCooldowns_=checkpoint.combat.procCooldowns;
  speedBuff_=checkpoint.combat.speedBuff;powerBuff_=checkpoint.combat.powerBuff;
  shieldTime_=checkpoint.combat.shieldTime;bodyProcCooldown_=checkpoint.combat.bodyProcCooldown;
  critCooldown_=checkpoint.combat.critCooldown;resetTimer_=checkpoint.combat.resetTimer;
  terrainTimer_=checkpoint.combat.terrainTimer;
  lowHpUsed_=checkpoint.combat.lowHpUsed;lowHpActive_=checkpoint.combat.lowHpActive;
  state_.wave=run.wave;state_.waveCount=run.mode==GameMode::Traditional?10:0;
  state_.waveDuration=ScheduleWave(run.mode,run.wave,checkpoint.campaignConfig).seconds;
  state_.waveElapsed=0;state_.phase=RunPhase::Wave;
  Sync();
  if(run.phase==CampaignPhase::Boss)StartBoss();
  return true;
}
bool Simulation::SealUpgrade(std::size_t index) {
  if(!fullGame_ || state_.paused || (state_.phase!=RunPhase::Wave && state_.phase!=RunPhase::Boss && state_.phase!=RunPhase::Rest))return false;
  bool ok=progression_.SealOffer(index);Sync();return ok;
}
bool Simulation::SkipUpgrade() {
  if(!fullGame_ || state_.paused || (state_.phase!=RunPhase::Wave && state_.phase!=RunPhase::Boss && state_.phase!=RunPhase::Rest))return false;
  bool ok=progression_.SkipUpgrade();if(ok)economy_.CreditStones(progression_.TakeSkipStones());Sync();return ok;
}
bool Simulation::BuyGood(std::uint64_t token) {
  bool ok=fullGame_ && !state_.paused && state_.phase==RunPhase::Rest && economy_.Buy(progression_,token); Sync(); return ok;
}
bool Simulation::RefreshShop() {
  bool ok=fullGame_ && !state_.paused && state_.phase==RunPhase::Rest && economy_.Refresh(progression_); Sync(); return ok;
}
bool Simulation::BeginReforge(EntityId id, std::size_t affix) {
  bool ok=fullGame_ && !state_.paused && state_.phase==RunPhase::Rest && economy_.BeginReforge(progression_,id,affix); Sync(); return ok;
}
bool Simulation::ResolveReforge(bool keepNew) {
  bool ok=fullGame_ && !state_.paused && state_.phase==RunPhase::Rest && economy_.ResolveReforge(progression_,keepNew); Sync(); return ok;
}
bool Simulation::OpenTreasure(bool paid) {
  // Free encounter openings are issued only by ChooseOpportunity, never by UI boolean.
  bool ok=fullGame_ && paid && !state_.paused && state_.phase==RunPhase::Rest && economy_.OpenTreasure(progression_,true); Sync(); return ok;
}
bool Simulation::RevealTreasure() {
  bool ok=fullGame_ && !state_.paused && state_.phase==RunPhase::Rest && economy_.RevealTreasure(progression_).has_value(); Sync(); return ok;
}
bool Simulation::IdentifyRelic(EntityId id) {
  bool ok=fullGame_ && !state_.paused && state_.phase==RunPhase::Rest && economy_.Identify(progression_,id); Sync(); return ok;
}
bool Simulation::Refine(const std::vector<EntityId> &ids, RefineReward reward) {
  if (state_.paused || state_.phase!=RunPhase::Rest) return false;
  bool ok=fullGame_ ? economy_.Refine(progression_,ids,reward) : reward==RefineReward::Experience && progression_.Refine(ids); Sync(); return ok;
}
bool Simulation::UndoRefine() {
  if (state_.paused || state_.phase!=RunPhase::Rest) return false;
  bool ok=fullGame_ ? economy_.UndoRefine(progression_) : progression_.UndoRefine(); Sync(); return ok;
}
bool Simulation::Equip(EntityId bagId, std::size_t slot) {
  if (state_.paused || state_.phase != RunPhase::Rest ||
      !progression_.Equip(bagId, slot)) return false;
  Sync(); return true;
}
bool Simulation::Unequip(std::size_t slot) {
  if (state_.paused || state_.phase != RunPhase::Rest ||
      !progression_.Unequip(slot)) return false;
  Sync(); return true;
}
bool Simulation::SetLocked(EntityId id, bool locked) {
  if (state_.paused || state_.phase != RunPhase::Rest ||
      !progression_.SetLocked(id, locked)) return false;
  Sync(); return true;
}
bool Simulation::SortBag() {
  if (state_.paused || state_.phase != RunPhase::Rest ||
      !progression_.IsResting()) return false;
  progression_.SortBag();
  Sync(); return true;
}
bool Simulation::CanFuse(SpellId result) const {
  if (state_.paused || (state_.phase != RunPhase::Wave &&
      state_.phase != RunPhase::Boss && state_.phase != RunPhase::Rest)) return false;
  // Upgrade waiting freezes combat independently of explicit pause, so fusion
  // and choice commands remain available under the existing rest/offer rules.
  return (state_.phase == RunPhase::Rest || !progression_.GetState().offers.empty()) &&
      (progression_.IsResting() || !progression_.GetState().offers.empty()) &&
      progression_.CanFuse(result);
}
bool Simulation::Fuse(SpellId result) {
  if (!CanFuse(result) || !progression_.Fuse(result)) return false;
  Sync(); return true;
}
void Simulation::Pause(bool p) {
  state_.paused = p;
  accumulator_ = 0;
}
void Simulation::Sync() {
  state_.spellCooldowns = spellCooldowns_;
  state_.progression = progression_.GetState();
  state_.player.hp =
      std::min(state_.player.hp, state_.progression.build.stats.maxHp);
}
void Simulation::RefreshProgression() { Sync(); }
bool Simulation::ChooseUpgrade(std::size_t i) {
  if (state_.paused ||
      (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss &&
       state_.phase != RunPhase::Rest))
    return false;
  bool ok = progression_.ChooseUpgrade(i);
  Sync();
  return ok;
}
bool Simulation::Reroll() {
  if (state_.paused ||
      (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss &&
       state_.phase != RunPhase::Rest))
    return false;
  bool ok = progression_.Reroll();
  Sync();
  return ok;
}
bool Simulation::BeginNextWave() {
  if (state_.paused || state_.phase != RunPhase::Rest ||
      progression_.GetState().pendingUpgrades > 0)
    return false;
  Campaign next = campaign_;
  const bool eliteEncounter = fullGame_ &&
      campaign_.GetState().phase == CampaignPhase::OpportunityCombat &&
      !opportunityEnemy_;
  if (fullGame_ && (!progression_.GetState().bossRewards.empty() ||
      economy_.HasTreasureToReveal() || economy_.PendingReforge() ||
      (!eliteEncounter && !next.BeginNextWave()))) return false;

  // Project deferred refinement XP before leaving rest. If it creates choices,
  // commit only the XP boundary and keep the inventory/encounter open.
  Progression prepared = progression_;
  Economy preparedEconomy = economy_;
  if (fullGame_) preparedEconomy.CommitRestExperience(prepared);
  else prepared.EndRest();
  if (prepared.GetState().pendingUpgrades > 0) {
    if (!fullGame_) prepared.BeginRest();
    progression_ = std::move(prepared);
    if (fullGame_) economy_ = std::move(preparedEconomy);
    Sync();
    return false;
  }
  if (eliteEncounter) {
    // Do not commit EndRest on a failed spawn. A retry must remain possible.
    opportunityEnemy_ = Spawn(EnemyRole::Charger, Edge(), EnemyTier::Elite);
    if (!opportunityEnemy_) return false;
  }
  if (fullGame_) preparedEconomy.EndRest(prepared);
  prepared.EndRest();
  progression_ = std::move(prepared);
  if (fullGame_) { economy_ = std::move(preparedEconomy); campaign_ = std::move(next); }
  if (eliteEncounter) {
    // The selected encounter is still part of this wave. No normal wave reset,
    // additional opportunity, or checkpoint advancement is permitted yet.
    progression_.RecordEncounter(EnemyRole::Charger, state_.enemies.back().mutations);
    state_.phase = RunPhase::Wave;
    accumulator_ = 0;
    Sync();
    return true;
  }
  Sync();
  lowHpUsed_ = lowHpActive_ = false;
  eliteSpawned_ = 0;
  waveSpawned_.fill(0);
  spawnAccumulator_ = 0;
  state_.waveElapsed = 0;
  accumulator_ = 0;
  if (fullGame_) {
    if (campaign_.GetState().phase == CampaignPhase::Boss) { StartBoss(); return true; }
    state_.wave = campaign_.GetState().wave;
    state_.waveDuration = ScheduleWave(campaign_.GetState().mode, state_.wave, campaign_.GetConfig()).seconds;
    if (campaign_.GetState().mode == GameMode::Quick) state_.waveDuration = config_.waveDurations[state_.wave-1];
    state_.phase = RunPhase::Wave; beastRolled_ = beastPending_ = false; return true;
  }
  if (state_.wave >= state_.waveCount) {
    StartBoss();
    return true;
  }
  ++state_.wave;
  state_.waveDuration = config_.waveDurations[state_.wave - 1];
  state_.phase = RunPhase::Wave;
  return true;
}
void Simulation::Step(float dt, const InputState &input) {
  if (!std::isfinite(dt) || dt <= 0)
    return;
  if (IsCombatFrozen()) {
    accumulator_ = 0;
    dashLatched_ = input.dash;
    dashQueued_ = false;
    return;
  }
  if (input.dash && !dashLatched_) {
    dashQueued_ = true;
  }
  dashLatched_ = input.dash;
  accumulator_ += std::min(dt, .25f);
  int budget = 64;
  while (accumulator_ + 1e-9 >= config_.fixedStep && budget-- > 0) {
    InputState stepInput = input;
    stepInput.dash = dashQueued_;
    dashQueued_ = false;
    Tick(config_.fixedStep, stepInput);
    accumulator_ -= config_.fixedStep;
    // Tick can create offers through pickup XP or overflow death rewards.
    // Read progression directly: the presentation snapshot need not be synced.
    if (IsCombatFrozen()) {
      accumulator_ = 0;
      dashQueued_ = false;
      dashLatched_ = input.dash;
      break;
    }
  }
  Sync();
}
void Simulation::Tick(float dt, const InputState &input) {
  currentSource_ = SpellId::Count;
  ++state_.tick;
  state_.statistics.elapsed += dt;
  state_.waveElapsed += dt;
  auto &p = state_.player;
  const auto &stats = state_.progression.build.stats;
  Down(p.invulnerable, dt);
  Down(p.dashCooldown, dt);
  Down(speedBuff_, dt);
  Down(powerBuff_, dt);
  Down(shieldTime_, dt);
  Down(bodyProcCooldown_, dt);
  Down(critCooldown_, dt);
  for (auto &t : procCooldowns_)
    Down(t, dt);
  if (shieldTime_ == 0)
    p.shield = 0;
  Vec2 movement = input.move;
  if (!std::isfinite(movement.x) || !std::isfinite(movement.y))
    movement = {};
  if (Len2(movement) > 1)
    movement = Unit(movement);
  if (Len2(movement) > .001f)
    p.facing = Unit(movement);
  if (input.dash && p.dashCooldown <= 0) {
    p.dashRemaining = .3f;
    p.invulnerable = std::max(p.invulnerable, .3f);
    p.dashCooldown = std::max(2.f, stats.dashCooldown);
    if (progression_.HasTrigger(3))
      powerBuff_ = 2;
    EffectState e;
    e.kind = EffectKind::Dash;
    e.position = p.position;
    e.radius = .7f;
    e.life = .3f;
    AddEffect(e);
    if (progression_.HasMystery(1)) {
      e.kind = EffectKind::Burn;
      e.radius = 2;
      e.life = 3;
      e.damage = 4;
      e.interval = .5f;
      e.oneShot = false;
      e.element = Element::Fire;
      AddEffect(e);
    }
  }
  float terrainMove=1;
  if(fullGame_)for(const auto &effect:state_.effects)
    if(effect.hostile && effect.element==Element::Wood && effect.form==Form::Array && effect.warning<=0 && effect.life>0 &&
       Len2(p.position-effect.position)<=effect.radius*effect.radius)
      terrainMove=1-BossRules().rootSlow;
  bool wasDash = p.dashRemaining > 0;
  if (wasDash) {
    float travel = std::min(dt, p.dashRemaining);
    p.position = Constrain(p.position + p.facing * (4.f / .3f * travel));
    Down(p.dashRemaining, dt);
    if (p.dashRemaining == 0 && progression_.HasMystery(2))
      Area(p.position, 2, 40, Element::Thunder, Form::None, false);
  } else
    p.position =
        Constrain(p.position +
              movement * (stats.moveSpeed * terrainMove * (speedBuff_ > 0 ? 1.15f : 1) * dt));
  p.hp = std::min(stats.maxHp, p.hp + stats.regen * dt);
  if (progression_.HasTrigger(7) && !lowHpUsed_ && p.hp < stats.maxHp * .3f) {
    lowHpActive_ = lowHpUsed_ = true;
  }
  if (p.hp >= stats.maxHp * .5f)
    lowHpActive_ = false;
  if (progression_.HasUnique(1)) {
    resetTimer_ += dt;
    if (resetTimer_ >= 20) {
      resetTimer_ -= 20;
      spellCooldowns_.fill(0);
    }
  }
  if (fullGame_) {
    terrainTimer_ += dt;
    if (terrainTimer_ >= 12 && !arena_.hazards.empty()) {
      terrainTimer_ -= 12;
      EffectState trap;trap.hostile=true;trap.warning=1.2f;trap.life=1.5f;
      trap.position=arena_.hazards[std::min(arena_.hazards.size()-1,std::size_t(Random()*arena_.hazards.size()))];
      trap.radius=2;trap.damage=12*GetTrialEffects(campaign_.GetState().riskMask, campaign_.GetConfig()).hazard;
      trap.kind=arena_.cave==CaveId::EmberMountain ? EffectKind::Burn : arena_.cave==CaveId::ThunderMarsh ? EffectKind::Lightning : EffectKind::Explosion;
      trap.element=arena_.cave==CaveId::EmberMountain ? Element::Fire : Element::None;
      AddEffect(trap);
    }
    for (const auto &event : anomalies_.Step(dt, p.position, state_.progression.build.resonance.thunder > 0)) {
      if (event.warning) {
        EffectState fx; fx.position=event.position; fx.radius=event.radius;
        fx.kind=EffectKind::Lightning; fx.warning=anomalies_.Config().warning; fx.life=.2f;fx.hostile=true;
        anomalyWarningShown_=AddEffect(fx);
      } else {
        if (anomalyWarningShown_ && !event.absorbed && Len2(p.position-event.position) <= event.radius*event.radius) Hurt(event.damage, event.anomaly == AnomalyId::Fissure ? DamageKind::Fissure : DamageKind::Lightning, 0, event.position);
        if (event.anomaly == AnomalyId::Fissure)
          Area(event.position,event.radius,event.damage,Element::None,Form::None,false);
      }
    }
    if (state_.phase == RunPhase::Dead) return;
  }
  if (state_.phase == RunPhase::Wave && !opportunityEnemy_) {
    SpawnWave(dt);
  }
  UpdateEnemies(dt);
  if (state_.phase == RunPhase::Dead)
    return;
  UpdateSpells(dt);
  UpdateProjectiles(dt);
  if(state_.phase==RunPhase::Dead)return;
  UpdateEffects(dt);
  if(state_.phase==RunPhase::Dead)return;
  ResolveDeaths();
  if (IsAwaitingUpgrade()) return;
  CollectPickups();
  if (IsAwaitingUpgrade()) return;
  if (state_.phase == RunPhase::Wave && !opportunityEnemy_ &&
      state_.waveElapsed >= state_.waveDuration)
    EndWave();
}
Vec2 Simulation::Edge() {
  if (fullGame_ && !arena_.edgeSpawns.empty()) return arena_.edgeSpawns[std::min(arena_.edgeSpawns.size()-1, std::size_t(Random()*arena_.edgeSpawns.size()))];
  float t = Random();
  if (Random() < .5f)
    return {Random() < .5f ? -47.f : 47.f, -26.f + t * 52};
  return {-47.f + t * 94, Random() < .5f ? -26.f : 26.f};
}
EnemyState *Simulation::Find(EntityId id) {
  for (auto &e : state_.enemies)
    if (e.id == id)
      return &e;
  return nullptr;
}
EnemyState *Simulation::Target(bool highest, float range) {
  EnemyState *best = nullptr;
  float d = range * range;
  for (auto &e : state_.enemies) {
    if (e.hp <= 0)
      continue;
    float q = Len2(e.position - state_.player.position);
    if (q > range * range)
      continue;
    if (!best ||
        (highest ? (e.hp > best->hp || (e.hp == best->hp && q < d)) : q < d)) {
      best = &e;
      d = q;
    }
  }
  return best;
}
Vec2 Simulation::DenseTarget(float radius) {
  Vec2 at = state_.player.position;
  int high = 0;
  for (const auto &e : state_.enemies) {
    int n = 0;
    for (const auto &other : state_.enemies)
      if (other.hp > 0 && Len2(e.position - other.position) < radius * radius)
        ++n;
    if (n > high) {
      high = n;
      at = e.position;
    }
  }
  return at;
}
EntityId Simulation::Spawn(EnemyRole role, Vec2 pos, EnemyTier tier,
                           EntityId owner, bool ignorePhaseCap) {
  static constexpr int capsA[] = {40, 70, 100, 130};
  static constexpr int capsB[] = {30, 50, 70, 90, 110, 130};
  std::size_t phaseCap = state_.phase == RunPhase::Boss ? 80
                         : config_.sixWaves
                             ? capsB[std::clamp(state_.wave - 1, 0, 5)]
                             : capsA[std::clamp(state_.wave - 1, 0, 3)];
  if (fullGame_ && state_.phase != RunPhase::Boss)
    phaseCap = std::size_t(ScheduleWave(campaign_.GetState().mode, state_.wave, campaign_.GetConfig()).enemyCap * GetTrialEffects(campaign_.GetState().riskMask, campaign_.GetConfig()).population);
  std::size_t cap = ignorePhaseCap ? config_.maxEnemies
                                   : std::min(config_.maxEnemies, phaseCap);
  if (state_.enemies.size() >= cap) {
    ++state_.statistics.droppedEvents;
    return 0;
  }
  EnemyState e;
  e.id = Id();
  e.role = role;
  e.tier = tier;
  e.position = Constrain(pos);
  e.owner = owner;
  int i = int(role);
  int w = std::clamp(state_.wave - 1, 0, 5);
  const float hp4[] = {1, 1.5f, 2.2f, 3.2f};
  const float hp6[] = {1, 1.3f, 1.7f, 2.2f, 2.7f, 3.2f};
  float h = config_.sixWaves ? hp6[w] : hp4[std::min(w, 3)];
  const float damage4[] = {1, 1.15f, 1.3f, 1.5f};
  float d = config_.sixWaves ? 1 + .5f * float(w) / 5 : damage4[std::min(w, 3)];
  if (fullGame_) {
    auto schedule = ScheduleWave(campaign_.GetState().mode, state_.wave, campaign_.GetConfig());
    h = schedule.healthScale; d = schedule.damageScale;
  }
  e.hp = e.maxHp = EnemyHp[i] * h;
  e.speed = EnemySpeed[i];
  e.damage = EnemyDamage[i] * d;
  if (fullGame_) {
    e.speed *= GetTrialEffects(campaign_.GetState().riskMask, campaign_.GetConfig()).speed;
  }
  e.cooldown = (role == EnemyRole::Swarm ? 0 : 1);
  if (tier == EnemyTier::Elite) {
    e.hp *= 8;
    e.maxHp = e.hp;
    e.damage *= 1.5f;
    e.radius = .8f;
    e.mutations = 1 << int(Random() * 4);
    if (Random() < .5f)
      e.mutations |= 1 << int(Random() * 4);
    if (e.mutations & 1)
      e.speed *= 1.2f;
    if (e.mutations & 2)
      e.armor = 200;
  }
  if (role == EnemyRole::Boss) {
    e.hp = e.maxHp = config_.bossHp1 * (fullGame_ && campaign_.GetState().mode==GameMode::Endless ? ScheduleWave(GameMode::Endless,state_.wave,campaign_.GetConfig()).healthScale : 1);
    e.bossPhase = 1;
    e.radius = 1.8f;
  }
  e.position=Constrain(e.position,e.radius);
  state_.enemies.push_back(e);
  progression_.RecordEncounter(e.role, e.mutations);
  return e.id;
}
EntityId Simulation::TestSpawn(EnemyRole r, Vec2 p, EnemyTier t) {
  return Spawn(r, p, t, 0, true);
}
void Simulation::TestDamageEnemy(EntityId id, float d, Element element) {
  if (auto *e = Find(id))
    Hit(*e, d, element, Form::None, false);
  ResolveDeaths();
  Sync();
}
void Simulation::TestDamagePlayer(float damage) { Hurt(damage); }
void Simulation::SpawnWave(float dt) {
  (void)dt;
  if (!config_.spawning || state_.statistics.elapsed < config_.spawnGrace)
    return;
  int effective = config_.sixWaves ? std::min(3, (state_.wave - 1) * 4 / 6)
                                   : std::min(3, state_.wave - 1);
  int caps4[] = {40, 70, 100, 130};
  int caps6[] = {30, 50, 70, 90, 110, 130};
  int cap = config_.sixWaves ? caps6[std::min(5,state_.wave - 1)] : caps4[effective];
  WaveSchedule schedule;
  AnomalyModifiers modifiers;
  TrialEffects trials;
  if (fullGame_) {
    schedule = ScheduleWave(campaign_.GetState().mode, state_.wave, campaign_.GetConfig());
    modifiers = anomalies_.Modifiers(state_.player.position);
    trials = GetTrialEffects(campaign_.GetState().riskMask, campaign_.GetConfig());
    cap = std::min(int(config_.maxEnemies), int(schedule.enemyCap * trials.population));
  }
  float f = state_.waveElapsed / state_.waveDuration;
  // A won spirit-beast draw owns a pending spawn, not a second lottery.
  // Retry before ordinary mobs consume newly released capacity.
  if (beastPending_ && int(state_.enemies.size()) < cap &&
      Spawn(EnemyRole::SpiritBeast, Edge(), EnemyTier::Variant))
    beastPending_ = false;
  int desired = effective == 2 ? (f > .65f ? 1 : 0)
                : effective == 3 ? (f > .8f ? 2 : f > .5f ? 1 : 0) : 0;
  if (fullGame_) {
    const int total = int(schedule.eliteCount * trials.elite);
    desired = 0;
    for (int i = 0; i < total; ++i) {
      const float entry = total == 1 ? .65f : .5f + .3f * i / (total - 1);
      if (f > entry) ++desired;
    }
  }
  // Due elites get the next free slot before ordinary catch-up spawns. Their
  // scheduled entry times are mid/late-wave rather than the cleanup boundary.
  while (eliteSpawned_ < desired && int(state_.enemies.size()) < cap) {
    if (!Spawn(eliteSpawned_ == 1 ? EnemyRole::Ranged : EnemyRole::Charger,
               Edge(), EnemyTier::Elite)) break;
    ++eliteSpawned_;
  }
  // Per-role budgets are distributed over six opening segments, not weighted
  // away.
  static constexpr int budgets[4][6] = {{240, 10, 0, 0, 0, 0},
                                        {300, 20, 12, 4, 0, 0},
                                        {420, 25, 20, 8, 6, 0},
                                        {520, 30, 25, 12, 8, 5}};
  static constexpr float starts[6] = {0, .15f, .3f, .45f, .6f, .75f};
  int remaining = 12;
  for (int role = 5; role >= 0 && remaining > 0; --role) {
    float begin = starts[role] * state_.waveDuration;
    if (state_.wave == 1)
      begin = std::max(begin, config_.spawnGrace);
    float progress = std::clamp((state_.waveElapsed - begin) /
                                    std::max(.1f, state_.waveDuration - begin),
                                0.f, 1.f);
    float budget = float(fullGame_ ? schedule.roleCounts[role] : budgets[effective][role]);
    if (fullGame_) budget *= trials.population * modifiers.spawn * CaveCatalog()[int(arena_.cave)].roleWeights[role == 1 ? 3 : role == 2 ? 1 : role == 3 ? 2 : role] *
        (role == int(EnemyRole::Charger) ? modifiers.chargers : role == int(EnemyRole::Ranged) ? modifiers.ranged : 1);
    int desiredForRole = int(budget * progress);
    while (waveSpawned_[role] < desiredForRole && int(state_.enemies.size()) < cap &&
           remaining-- > 0) {
      if(!Spawn(EnemyRole(role), Edge(),
            role >= 2 ? EnemyTier::Variant : EnemyTier::Normal)) break;
      ++waveSpawned_[role];
    }
  }
  if (!beastRolled_ && (fullGame_ || effective == 1) && f > .6f) {
    beastRolled_ = true;
    float luck =
        40 * (1 - std::exp(-std::max(0.f, state_.progression.build.stats.luck) /
                           30));
    if (Random() < (fullGame_ ? .12f : .4f) * (1 + luck * .015f) * modifiers.beasts)
      beastPending_ = !Spawn(EnemyRole::SpiritBeast, Edge(), EnemyTier::Variant);
  }
}
bool Simulation::AddEffect(EffectState e) {
  e.id = Id();
  e.duration = e.life;
  if (state_.effects.size() >= config_.maxEffects) {
    ++state_.statistics.droppedEvents;
    // Warnings may replace cosmetics or already-fired hazards, never another
    // pending warning whose owner could still execute its attack.
    if (e.hostile && e.warning > 0) {
      auto slot = std::find_if(state_.effects.begin(), state_.effects.end(),
                               [](const EffectState &old) {
                                 return !old.hostile || old.warning <= 0;
                               });
      if (slot != state_.effects.end()) {
        *slot = e;
        return true;
      }
    }
    return false;
  }
  state_.effects.push_back(e);
  return true;
}
void Simulation::AddProjectile(ProjectileState p) {
  if (state_.projectiles.size() >= config_.maxProjectiles) {
    ++state_.statistics.droppedEvents;
    return;
  }
  p.id = Id();
  p.previous = p.position;
  state_.projectiles.push_back(p);
  ++state_.statistics.shots;
}
void Simulation::Hurt(float d, DamageKind kind, EntityId source, Vec2 position,
                      EffectKind effectKind) {
  // Empty warning visuals and exhausted one-shot effects are not hits. Nor may
  // invalid external/test input heal the player or poison combat state.
  if (!std::isfinite(d) || d <= 0) return;
  auto &p = state_.player;
  if (p.invulnerable > 0 || state_.phase == RunPhase::Dead ||
      state_.phase == RunPhase::Won) {
    ++state_.statistics.dodges;
    return;
  }
  const auto &s = state_.progression.build.stats;
  if (progression_.HasTrigger(2) && procCooldowns_[1] <= 0 && Random() < .08f) {
    p.shield += 20;
    shieldTime_ = 4;
    procCooldowns_[1] = 8;
  }
  d *= 1 - std::clamp(s.armor + (lowHpActive_ ? .15f : 0), 0.f, .6f);
  float shield = std::min(p.shield, d);
  p.shield -= shield;
  d -= shield;
  const float hpBefore = p.hp;
  p.hp = std::max(0.f, p.hp - d);
  state_.statistics.damageTaken += d;
  state_.lastDamage = {kind, effectKind, source, position, state_.tick,
                       hpBefore - p.hp, shield, p.hp <= 0};
  for (auto spell : state_.progression.build.spells)
    if (spell.id == SpellId::FlameBody && spell.rank == 5 &&
        procCooldowns_[0] <= 0) {
      procCooldowns_[0] = 6;
      Area(p.position, 3, 40, Element::Fire, Form::Body, false);
      break;
    }
  if (p.hp <= 0) {
    state_.phase = RunPhase::Dead;
    if (fullGame_) { campaign_.Die(); economy_.EndRun(); }
  }
}
float Simulation::ScaleDamage(float damage, Element element, Form form) const {
  const auto &b = state_.progression.build;
  const auto &s = b.stats;
  if (fullGame_) { auto m=anomalies_.Modifiers(state_.player.position); damage *= m.playerPower * (element==Element::Thunder ? m.thunderPower : 1); }
  damage *= (s.power + (powerBuff_ > 0 ? .15f : 0));
  if (element != Element::None)
    damage *= 1 + s.elementBonus[int(element)];
  if (form != Form::None)
    damage *= 1 + s.formBonus[int(form)];
  if (form == Form::Sword && config_.formResonance && b.resonance.sword >= 4 &&
      state_.statistics.elapsed >= 8 &&
      std::fmod(state_.statistics.elapsed, 8.f) < 3)
    damage *= 1.3f;
  return damage;
}
void Simulation::Hit(EnemyState &e, float damage, Element element, Form form,
                     bool primary, float burn, float duration, float stun) {
  if (e.hp <= 0 || damageDepth_ >= 3)
    return;
  ++damageDepth_;
  if (element == Element::Water && e.burnRemaining > 0) damage *= 1.25f;
  if (primary && currentSource_ != SpellId::Count) {
    int rank=1;for(auto spell:state_.progression.build.spells)if(spell.id==currentSource_)rank=spell.rank;
    auto source=GetSpellConfig(currentSource_,rank);
    if(source.healOnHit>0)state_.player.hp=std::min(state_.progression.build.stats.maxHp,state_.player.hp+source.healOnHit);
    if(source.slowFraction>0){e.slowRemaining=std::max(e.slowRemaining,source.slowDuration);e.slowFraction=std::max(e.slowFraction,source.slowFraction);}
    stun=std::max(stun,source.stun);
  }
  const auto &b = state_.progression.build;
  const auto &s = b.stats;
  bool crit = false;
  if (primary) {
    damage = ScaleDamage(damage, element, form);
    crit = Random() < s.critChance;
    if (crit)
      damage *= s.critMultiplier;
  }
  if (e.stun > 0) {
    for (const auto &fx : state_.effects)
      if (fx.prisonVulnerability && fx.warning <= 0 &&
          Len2(e.position - fx.position) <= fx.radius * fx.radius) {
        damage *= 1.2f;
        break;
      }
  }
  if (config_.formResonance && b.resonance.array >= 4) {
    for (const auto &fx : state_.effects)
      if (!fx.hostile && fx.form == Form::Array && fx.warning <= 0 &&
          Len2(e.position - fx.position) <= fx.radius * fx.radius) {
        damage *= 1.2f;
        break;
      }
  }
  float originalHp = e.hp;
  if (e.armor > 0) {
    damage *= .7f;
    float a = std::min(e.armor, damage);
    e.armor -= a;
    damage -= a;
  }
  if(fullGame_ && element==Element::Fire)e.shield=0;
  if (e.shield > 0) {
    float factor = element == Element::Fire ? 2.f : 1.f;
    float a = std::min(e.shield, damage * factor);
    e.shield -= a;
    damage -= a / factor;
  }
  e.hp -= std::max(0.f, damage);
  state_.statistics.damageDealt += std::min(originalHp, std::max(0.f, damage));
  ++state_.statistics.hits;
  if (burn > 0) {
    e.burnRemaining = std::max(e.burnRemaining, duration);
    e.burnStacks = std::min(b.resonance.fire >= 4 ? 3 : 1, e.burnStacks + 1);
    e.burnDps = std::max(
        e.burnDps, burn * (s.power + (powerBuff_ > 0 ? .15f : 0)) *
                       (1 + s.elementBonus[0]) *
                       (form == Form::None ? 1 : 1 + s.formBonus[int(form)]) *
                       s.burnMultiplier);
  }
  e.stun = std::max(e.stun, stun);
  if (element == Element::Thunder && e.role == EnemyRole::Exploder) {
    e.hp = 0;
    Area(e.position, 2.5f, 25, Element::None, Form::None, false);
  }
  if (primary && b.resonance.thunder >= 2 &&
      Random() < (b.resonance.thunder >= 4 ? .15f : .08f)) {
    Hit(e, 25, Element::None, Form::None, false, 0, 0,
        b.resonance.thunder >= 4 ? .5f : 0);
    EffectState fx;
    fx.kind = EffectKind::Lightning;
    fx.position = e.position;
    fx.radius = .7f;
    AddEffect(fx);
  }
  if (primary && form == Form::Body) {
    if (config_.formResonance && b.resonance.body >= 4 &&
        bodyProcCooldown_ <= 0 && Random() < .1f) {
      state_.player.invulnerable = std::max(state_.player.invulnerable, 1.f);
      bodyProcCooldown_ = 5;
    }
    if (progression_.HasMystery(5) && Random() < .12f)
      Area(e.position, 3, 30, Element::None, Form::None, false);
  }
  if (primary && crit) {
    if (progression_.HasTrigger(5) && procCooldowns_[4] <= 0 &&
        Random() < .1f) {
      procCooldowns_[4] = 1;
      Hit(e, 15, Element::None, Form::None, false);
    }
    if (progression_.HasMystery(6) && critCooldown_ <= 0) {
      critCooldown_ = .5f;
      for (int slot = 0; slot < int(spellCooldowns_.size()); ++slot)
        if (slot != Index(currentSource_))
          Down(spellCooldowns_[slot], .3f);
    }
  }
  --damageDepth_;
}
void Simulation::Area(Vec2 at, float radius, float damage, Element element,
                      Form form, bool primary, float burn, float duration,
                      float stun) {
  for (auto &e : state_.enemies)
    if (e.hp > 0 &&
        Len2(e.position - at) < (radius + e.radius) * (radius + e.radius))
      Hit(e, damage, element, form, primary, burn, duration, stun);
}
void Simulation::Chain(EntityId start, int count, float damage, float scale,
                       float radius, Form sourceForm, bool highest) {
  std::vector<EntityId> visited{start};
  auto *first = Find(start);
  if (!first)
    return;
  Vec2 at = first->position;
  for (int n = 0; n < count; ++n) {
    EnemyState *best = nullptr;
    float nearest = radius * radius;
    for (auto &e : state_.enemies) {
      float d = Len2(e.position - at);
      if (e.hp > 0 &&
          std::find(visited.begin(), visited.end(), e.id) == visited.end() &&
          d < radius * radius &&
          (!best || (highest ? e.hp > best->hp : d < nearest))) {
        nearest = d;
        best = &e;
      }
    }
    if (!best)
      break;
    EffectState fx;
    fx.kind = EffectKind::Line;
    fx.position = at;
    fx.end = best->position;
    fx.element = Element::Thunder;
    fx.life = .15f;
    AddEffect(fx);
    visited.push_back(best->id);
    at = best->position;
    Hit(*best,
        ScaleDamage(damage * scale, Element::Thunder, sourceForm) *
            state_.progression.build.stats.chainMultiplier,
        Element::Thunder, Form::None, false);
  }
}
void Simulation::UpdateEnemies(float dt) {
  const std::size_t initial = state_.enemies.size();
  for (std::size_t i = 0; i < initial; ++i) {
    auto &e = state_.enemies[i];
    if (e.hp <= 0)
      continue;
    if (e.burnRemaining > 0) {
      Down(e.burnRemaining, dt);
      Hit(e, e.burnDps * e.burnStacks * dt, Element::Fire, Form::None, false);
      if (e.burnRemaining == 0) {
        e.burnDps = 0;
        e.burnStacks = 0;
      }
    }
    if (e.hp <= 0)
      continue;
    Down(e.stun, dt);
    if (e.stun > 0)
      continue;
    Down(e.cooldown, dt);
    Down(e.slowRemaining, dt);
    if(e.slowRemaining<=0)e.slowFraction=0;
    const float attackDamage=e.damage*(fullGame_ && (e.role==EnemyRole::Swarm || e.role==EnemyRole::Exploder) ? anomalies_.Modifiers(state_.player.position).nightEnemyPower : 1.f);
    Vec2 delta = state_.player.position - e.position;
    float distance = Len(delta);
    Vec2 toward = Unit(delta);
    if (e.mutations & 8) {
      for (int orb = 0; orb < 3; ++orb) {
        float a = state_.statistics.elapsed * 2 + orb * 2 * Pi / 3;
        Vec2 at = e.position + Vec2{std::cos(a), std::sin(a)} * 1.7f;
        if (Len(at - state_.player.position) < .65f && e.cooldown <= 0) {
          Hurt(6, DamageKind::Orbit, e.id, at);
          e.cooldown = .8f;
        }
      }
    }
    if (e.role == EnemyRole::Boss) {
      if(fullGame_ && arena_.cave!=CaveId::ThunderMarsh) { UpdateCaveBoss(e,dt);continue; }
      if (e.windup <= 0 && e.chargeRemaining <= 0)
        e.position = Steer(e.position, toward, e.speed * (1-e.slowFraction) * dt * (distance > 5 ? 1.f : 0.f), e.radius);
      bossTimer_ += dt;
      bossSummon_ += dt;
      bossShield_ += dt;
      if (e.bossPhase == 1) {
        if (bossTimer_ >= 6) {
          bossTimer_ -= 6;
          for (int k = 0; k < 3; ++k) {
            EffectState fx;
            fx.kind = EffectKind::Lightning;
            float a = k * 2 * Pi / 3;
            fx.position = state_.player.position +
                          Vec2{std::cos(a), std::sin(a)} * (k ? 2.5f : 0);
            fx.radius = 1.7f;
            fx.warning = 1;
            fx.life = .2f;
            fx.damage = 35;
            fx.hostile = true;
            AddEffect(fx);
          }
        }
        if (bossSummon_ >= 5) {
          bossSummon_ -= 5;
          for (int k = 0; k < 20 && state_.enemies.size() < 80; ++k)
            Spawn(EnemyRole::Swarm, Edge(), EnemyTier::Normal);
        }
        if (bossShield_ >= 15) {
          bossShield_ -= 15;
          for (auto &other : state_.enemies)
            if (other.id != e.id)
              other.shield = 40;
        }
      } else {
        wallTimer_ += dt;
        chargeTimer_ += dt;
        if (wallTimer_ >= 8) {
          wallTimer_ -= 8;
          EffectState fx;
          fx.kind = EffectKind::FireWall;
          fx.position = {0, 0};
          fx.radius = 48;
          fx.warning = 1.5f;
          fx.life = 6;
          fx.interval = .25f;
          fx.damage = 5;
          fx.oneShot = false;
          fx.hostile = true;
          AddEffect(fx);
        }
        if (chargeTimer_ >= 10 && e.windup <= 0 && e.chargeRemaining <= 0) {
          chargeTimer_ -= 10;
          e.windup = 1.2f;
          e.direction = toward;
          EffectState fx;
          fx.kind = EffectKind::Line;
          fx.position = e.position;
          fx.end = e.position + toward * 20;
          fx.owner = e.id;
          fx.radius = 1.8f;
          fx.warning = 1.2f;
          fx.life = .15f;
          fx.hostile = true;
          if (!AddEffect(fx)) {
            e.windup = 0;
            e.cooldown = .25f;
          }
        }
        if (bossSummon_ >= 8) {
          bossSummon_ = 0;
          for (int k = 0; k < 10 && state_.enemies.size() < 80; ++k)
            Spawn(k < 4 ? EnemyRole::Ranged : EnemyRole::Charger, Edge(),
                  EnemyTier::Variant);
        }
      }
    }
    if (e.windup > 0) {
      Down(e.windup, dt);
      if (e.windup == 0) {
        if (e.role == EnemyRole::Exploder) {
          if (distance < 2.5f + .4f)
            Hurt(attackDamage, DamageKind::Explosion, e.id, e.position);
          e.hp = 0;
          e.burnRemaining = 0;
        } else if (e.role == EnemyRole::Charger || e.role == EnemyRole::Boss)
          e.chargeRemaining = 1.4f;
        else if (e.role == EnemyRole::Ranged) {
          ProjectileState p;
          p.owner = e.id;
          p.position = e.position;
          p.velocity = Unit(e.target - e.position) * 7;
          p.damage = attackDamage;
          p.hostile = true;
          p.life = Len(e.target - e.position) / 7;
          p.radius = .35f;
          AddProjectile(p);
        } else if (e.role == EnemyRole::Summoner) {
          int children = 0;
          for (const auto &o : state_.enemies)
            if (o.owner == e.id && o.hp > 0)
              ++children;
          for (int k = 0; k < std::min(3, 12 - children); ++k)
            Spawn(EnemyRole::Swarm,
                  e.position + Vec2{Random() * 2 - 1, Random() * 2 - 1},
                  EnemyTier::Normal, e.id);
        } else if (e.role == EnemyRole::Support) {
          int n = 0;
          for (auto &o : state_.enemies)
            if (o.id != e.id && o.hp > 0 &&
                Len2(o.position - e.position) < 64 && n++ < 3)
              o.shield = 40;
        }
      }
      continue;
    }
    if (e.chargeRemaining > 0) {
      Vec2 old = e.position;
      e.position = Steer(e.position, e.direction, 9 * dt, e.radius);
      Down(e.chargeRemaining, dt);
      if (SegmentDistance(state_.player.position, old, e.position) <
          e.radius + .4f) {
        Hurt(e.role == EnemyRole::Boss ? 45 : attackDamage, DamageKind::Charge, e.id, e.position);
        e.chargeRemaining = 0;
      }
      continue;
    }
    if (e.role == EnemyRole::Boss)
      continue;
    if (e.role == EnemyRole::SpiritBeast) {
      if (fullGame_ && !arena_.beastRoute.empty()) {
        if (Len2(e.target) < .01f || Len2(e.target-e.position)<1) {
          std::size_t closest=0; float best=1e9f;
          for(std::size_t k=0;k<arena_.beastRoute.size();++k) {
            float d=Len2(arena_.beastRoute[k]-e.position);
            if(d<best){best=d;closest=k;}
          }
          e.target=arena_.beastRoute[(closest+1)%arena_.beastRoute.size()];
        }
        e.position=Steer(e.position,Unit(e.target-e.position),e.speed*(1-e.slowFraction)*dt,e.radius);
        continue;
      }
      e.position = Steer(e.position, toward * -1, e.speed * (1-e.slowFraction) * dt, e.radius);
      continue;
    }
    if (e.role == EnemyRole::Swarm) {
      e.position = Steer(e.position, toward, e.speed * (1-e.slowFraction) * dt, e.radius);
      if (distance < e.radius + .4f && e.cooldown <= 0) {
        Hurt(attackDamage, DamageKind::Contact, e.id, e.position);
        e.cooldown = .8f;
      }
    }
    if (e.role == EnemyRole::Exploder) {
      e.position = Steer(e.position, toward, e.speed * (1-e.slowFraction) * dt, e.radius);
      if (distance < 2.5f) {
        e.windup = .8f;
        EffectState fx;
        fx.owner = e.id;
        fx.kind = EffectKind::Explosion;
        fx.position = e.position;
        fx.radius = 2.5f;
        fx.warning = .8f;
        fx.life = .15f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
    if (e.role == EnemyRole::Charger) {
      e.position = Steer(e.position, toward, e.speed * (1-e.slowFraction) * dt, e.radius);
      if (distance < 12 && e.cooldown <= 0) {
        e.windup = 1;
        e.direction = toward;
        e.cooldown = (e.mutations & 1) ? 2.8f : 4;
        EffectState fx;
        fx.owner = e.id;
        fx.kind = EffectKind::Line;
        fx.position = e.position;
        fx.end = e.position + toward * 12.6f;
        fx.radius = e.radius;
        fx.warning = 1;
        fx.life = .15f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
    if (e.role == EnemyRole::Ranged) {
      if (distance > 8)
        e.position = Steer(e.position, toward, e.speed * (1-e.slowFraction) * dt, e.radius);
      else if (distance < 5)
        e.position = Steer(e.position, toward * -1, e.speed * (1-e.slowFraction) * dt, e.radius);
      if (distance <= 9 && e.cooldown <= 0) {
        e.windup = .6f;
        e.target = state_.player.position;
        e.cooldown = (e.mutations & 1) ? 1.4f : 2;
        EffectState fx;
        fx.owner = e.id;
        fx.position = e.target;
        fx.radius = 1;
        fx.warning = .6f;
        fx.life = .15f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
    if (e.role == EnemyRole::Support || e.role == EnemyRole::Summoner) {
      if (distance > 7)
        e.position = Steer(e.position, toward, e.speed * (1-e.slowFraction) * dt, e.radius);
      if (e.cooldown <= 0) {
        e.windup = .6f;
        e.cooldown = e.role == EnemyRole::Support ? 5 : 4;
        EffectState fx;
        fx.owner = e.id;
        fx.kind = EffectKind::Shield;
        fx.position = e.position;
        fx.radius = e.role == EnemyRole::Support ? 8 : 2;
        fx.warning = .6f;
        fx.life = .2f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
  }
}
void Simulation::UpdateSpells(float dt) {
  for (auto &cd : spellCooldowns_)
    Down(cd, dt);
  if (!config_.automaticSpells)
    return;
  for (const auto spell : state_.progression.build.spells)
    if (spellCooldowns_[Index(spell.id)] <= 0) {
      Cast(spell);
    }
}
void Simulation::Cast(SpellState spell) {
  currentSource_ = spell.id;
  auto c = GetSpellConfig(spell.id, spell.rank);
  const auto &b = state_.progression.build;
  const auto &stats = b.stats;
  float radius = c.radius * stats.area;
  spellCooldowns_[Index(spell.id)] =
      std::max(.08f, c.cooldown * ((spell.id == SpellId::FlameBody ||
                                    spell.id == SpellId::FlameRing)
                                       ? 1.f
                                       : 1 - stats.cooldownReduction));
  ++casts_[Index(spell.id)];
  Vec2 at = state_.player.position;
  auto field = [&](Vec2 pos, float r, float damage, float warning, float life,
                   bool oneShot, float interval) {
    EffectState fx;
    fx.kind =
        c.element == Element::Fire ? EffectKind::Burn : EffectKind::Lightning;
    fx.position = pos;
    fx.radius = r;
    fx.warning = warning;
    fx.life = life;
    fx.damage = damage;
    fx.interval = interval;
    fx.oneShot = oneShot;
    fx.element = c.element;
    fx.form = c.form;
    fx.source = spell.id;
    fx.prisonVulnerability = c.prisonVulnerability;
    fx.burnDps = c.burnDps;
    fx.burnDuration = c.burnDuration;
    fx.stun = c.stun;
    fx.followPlayer =
        c.followPlayer || (c.form == Form::Array && progression_.HasMystery(4));
    if (c.form == Form::Array && progression_.HasMystery(4))
      fx.radius *= .8f;
    AddEffect(fx);
  };
  if (c.behavior != SpellBehavior::Legacy) {
    if (c.shieldOnCast > 0) { state_.player.shield += c.shieldOnCast; shieldTime_ = std::max(shieldTime_, c.duration > 0 ? c.duration : 3.f); }
    if (c.behavior == SpellBehavior::Cone) {
      for(auto &enemy:state_.enemies) {
        Vec2 d=enemy.position-at; float length=Len(d);
        float dot=length>0 ? (d.x*state_.player.facing.x+d.y*state_.player.facing.y)/length : 1;
        if(length<=radius+enemy.radius && dot>=std::cos(c.coneDegrees*.5f*Pi/180))
          Hit(enemy,c.damage,c.element,c.form,true,c.burnDps,c.burnDuration,c.stun);
      }
      EffectState fx;fx.kind=EffectKind::Slash;fx.position=at;fx.end=at+state_.player.facing*radius;
      fx.radius=radius;fx.life=.2f;fx.source=spell.id;fx.element=c.element;fx.form=c.form;AddEffect(fx);
    } else if (c.behavior == SpellBehavior::Field) {
      field(c.followPlayer ? at : DenseTarget(radius), radius,c.damage,c.warning,
          std::max(.1f,c.duration)*(config_.formResonance && b.resonance.array>=2 ? 1.25f : 1.f),c.duration<=0,std::max(.1f,c.interval));
    } else if (c.behavior == SpellBehavior::Companion) {
      // Count is companion count; radius in the catalog is its targeting range.
      for(int k=0;k<c.count;++k) {
        EffectState fx;fx.kind=EffectKind::Circle;fx.position=at;fx.radius=.6f;
        fx.life=std::max(.1f,c.duration);fx.interval=std::max(.1f,c.interval);fx.damage=c.damage;
        fx.oneShot=false;fx.followPlayer=true;fx.source=spell.id;fx.element=c.element;fx.form=c.form;
        fx.burnDps=c.burnDps;fx.burnDuration=c.burnDuration;fx.stun=c.stun;AddEffect(fx);
      }
    } else if (c.behavior == SpellBehavior::Talisman) {
      EnemyState *fastest=nullptr;
      for(auto &enemy:state_.enemies)if(enemy.hp>0 && Len2(enemy.position-at)<=14*14 &&
          (!fastest || enemy.speed>fastest->speed))fastest=&enemy;
      if(!fastest){spellCooldowns_[Index(spell.id)]=.15f;--casts_[Index(spell.id)];return;}
      for(int k=0;k<c.count;++k)
        field(fastest->position,radius,c.damage,c.warning+.12f*k,.15f,true,.5f);
    } else {
      EnemyState *target=nullptr;
      if(c.behavior==SpellBehavior::Talisman) {
        for(auto &enemy:state_.enemies) if(enemy.hp>0 && Len2(enemy.position-at)<=radius*radius &&
            (!target || enemy.speed>target->speed)) target=&enemy;
      } else target=Target(true,radius);
      if(!target){spellCooldowns_[Index(spell.id)]=.15f;--casts_[Index(spell.id)];return;}
      int extra=progression_.HasMystery(3) && c.form==Form::Sword && casts_[Index(spell.id)]%5==0 ? 2 : 0;
      for(int k=0;k<c.count+extra;++k) {
        ProjectileState p;p.position=at;p.source=spell.id;p.damage=c.damage*(k>=c.count?.6f:1.f);p.target=target->id;
        p.element=c.element;p.form=c.form;p.homing=true;p.pierce=c.pierce+(config_.formResonance&&c.form==Form::Sword&&b.resonance.sword>=2?1:0);p.returning=c.returning;
        p.chains=c.chains;p.chainScale=c.chainScale;p.burnDps=c.burnDps;p.burnDuration=c.burnDuration;
        float spread=(k-(c.count-1)*.5f)*.12f; Vec2 d=Unit(target->position-at);
        p.velocity={14*(d.x*std::cos(spread)-d.y*std::sin(spread)),14*(d.x*std::sin(spread)+d.y*std::cos(spread))};
        p.speed=14;p.life=radius/14;AddProjectile(p);
      }
    }
    return;
  }
  if (spell.id == SpellId::FlamePalm || spell.id == SpellId::ThunderFist) {
    EntityId first = 0;
    for (auto &e : state_.enemies) {
      Vec2 delta = e.position - at;
      float length = Len(delta);
      float dot = length > 0 ? (delta.x * state_.player.facing.x +
                                delta.y * state_.player.facing.y) /
                                   length
                             : 1;
      if (e.hp > 0 && length < radius + e.radius &&
          dot >= std::cos(c.coneDegrees * .5f * Pi / 180)) {
        Hit(e, c.damage, c.element, c.form, true, c.burnDps, c.burnDuration);
        if (!first)
          first = e.id;
      }
    }
    if (!first) {
      // Automatic melee waits for a legal facing-cone target, like sword
      // targeting.
      spellCooldowns_[Index(spell.id)] = .15f;
      --casts_[Index(spell.id)];
      return;
    }
    if (first && c.chains)
      Chain(first, c.chains, c.damage, c.chainScale, c.chainRadius, c.form,
            c.highestHpChain);
    EffectState fx;
    fx.kind = EffectKind::Slash;
    fx.position = at;
    fx.end = at + state_.player.facing * radius;
    fx.radius = radius;
    fx.element = c.element;
    fx.form = c.form;
    fx.source = spell.id;
    fx.life = .2f;
    AddEffect(fx);
    if (c.fireAfterwave && first) {
      EffectState after;
      after.kind = EffectKind::Burn;
      after.position = at;
      after.radius = radius;
      after.damage = 2;
      after.life = 2;
      after.interval = .5f;
      after.oneShot = false;
      after.element = Element::Fire;
      after.form = Form::Body;
      after.source = spell.id;
      AddEffect(after);
    }
    return;
  }
  if (spell.id == SpellId::FlameBody || spell.id == SpellId::FusionPalm) {
    if (spell.id == SpellId::FusionPalm && !Target(false, radius + .8f)) {
      spellCooldowns_[Index(spell.id)] = .15f;
      --casts_[Index(spell.id)];
      return;
    }
    Area(at, radius, c.damage, c.element, c.form, true, c.burnDps,
         c.burnDuration, c.stun);
    EffectState fx;
    fx.kind = spell.id == SpellId::FusionPalm ? EffectKind::Explosion
                                              : EffectKind::Burn;
    fx.position = at;
    fx.radius = radius;
    fx.life = .2f;
    fx.element = c.element;
    AddEffect(fx);
    return;
  }
  if (spell.id == SpellId::FlameRing) {
    if (c.expandingRing && state_.statistics.elapsed >= 6 &&
        std::fmod(state_.statistics.elapsed, 6.f) < 1.5f)
      radius = 5 * stats.area;
    int count =
        c.count + (config_.formResonance && b.resonance.sword >= 2 ? 1 : 0);
    for (int k = 0; k < count; ++k) {
      float angle = state_.statistics.elapsed * 3 * c.rotationMultiplier +
                    k * 2 * Pi / count;
      Vec2 pos = at + Vec2{std::cos(angle), std::sin(angle)} * radius;
      Area(pos, 1.2f, c.damage, c.element, c.form);
      EffectState fx;
      fx.kind = EffectKind::Slash;
      fx.position = pos;
      fx.radius = .6f;
      fx.life = .5f;
      fx.element = c.element;
      fx.form = c.form;
      fx.source = spell.id;
      AddEffect(fx);
    }
    return;
  }
  if (spell.id == SpellId::FlameSword || spell.id == SpellId::ThunderSword ||
      spell.id == SpellId::FusionSwords) {
    auto *target = Target(spell.id != SpellId::ThunderSword, c.radius);
    if (!target) {
      spellCooldowns_[Index(spell.id)] = .15f;
      --casts_[Index(spell.id)];
      return;
    }
    int count = c.count;
    int extra =
        progression_.HasMystery(3) && casts_[Index(spell.id)] % 5 == 0 ? 2 : 0;
    for (int k = 0; k < count + extra; ++k) {
      EnemyState *aim = target;
      if (k > 0 && c.count > 1) {
        EnemyState *alternative = nullptr;
        float near = 1e9f;
        for (auto &e : state_.enemies)
          if (e.hp > 0 && e.id != target->id &&
              Len2(e.position - at) <= c.radius * c.radius && Len2(e.position - at) < near) {
            near = Len2(e.position - at);
            alternative = &e;
          }
        if (alternative)
          aim = alternative;
      }
      float spread = (k - (count - 1) * .5f) * .12f;
      Vec2 dir = Unit(aim->position - at);
      dir = {dir.x * std::cos(spread) - dir.y * std::sin(spread),
             dir.x * std::sin(spread) + dir.y * std::cos(spread)};
      ProjectileState p;
      p.position = at;
      p.source = spell.id;
      p.pathBurn = spell.id == SpellId::FlameSword;
      if (spell.id == SpellId::FusionSwords) {
        p.orbitRemaining = .3f;
        p.orbitAngle = k * Pi;
      }
      p.velocity = dir * 14;
      p.speed = 14;
      p.turnMultiplier = c.turnMultiplier;
      p.target = aim->id;
      p.damage = c.damage * (k >= count ? .6f : 1);
      p.life = c.radius / 14;
      p.pierce =
          c.pierce + (config_.formResonance && b.resonance.sword >= 2 ? 1 : 0);
      p.element = c.element;
      p.form = c.form;
      p.homing = spell.id != SpellId::FlameSword;
      p.returning = c.returning;
      p.chains = c.chains;
      p.chainScale = c.chainScale;
      p.burnDps = c.burnDps;
      p.burnDuration = c.burnDuration;
      AddProjectile(p);
    }
    return;
  }
  if (spell.id == SpellId::SwordRain) {
    Vec2 center = DenseTarget(radius);
    int count =
        c.count + (config_.formResonance && b.resonance.sword >= 2 ? 1 : 0);
    int extra =
        progression_.HasMystery(3) && casts_[Index(spell.id)] % 5 == 0 ? 2 : 0;
    for (int k = 0; k < count + extra; ++k) {
      float angle = Random() * Pi * 2, r = std::sqrt(Random()) * radius;
      Vec2 pos = center + Vec2{std::cos(angle), std::sin(angle)} * r;
      field(pos, 1.2f, c.damage * (k >= count ? .6f : 1.f), c.warning, .12f,
            true, .5f);
    }
    if (c.electricAfterfield)
      field(center, radius, 2.5f, c.warning, 2, false, .5f);
    return;
  }
  if (c.form == Form::Array) {
    Vec2 center = spell.id == SpellId::FlameArray ? at : DenseTarget(radius);
    float duration =
        c.duration *
        (config_.formResonance && b.resonance.array >= 2 ? 1.25f : 1);
    if (spell.id == SpellId::HeavenlyThunder) {
      field(center, radius, c.damage, c.warning, .2f, true, .5f);
      for (int k = 0; k < c.afterBolts; ++k)
        field(center, radius, c.damage * c.afterBoltScale,
              c.warning + .4f * (k + 1), .15f, true, .5f);
    } else
      field(center, radius, c.damage, 0, duration, false, c.interval);
  }
}
void Simulation::UpdateProjectiles(float dt) {
  for (auto &p : state_.projectiles) {
    if (p.life <= 0)
      continue;
    currentSource_ = p.source;
    p.previous = p.position;
    if (p.orbitRemaining > 0) {
      Down(p.orbitRemaining, dt);
      p.orbitAngle += dt * 16;
      p.position = state_.player.position +
                   Vec2{std::cos(p.orbitAngle), std::sin(p.orbitAngle)} * 1.5f;
      continue;
    }
    if (p.homing) {
      auto *target = Find(p.target);
      if (target && target->hp > 0) {
        Vec2 desired = Unit(target->position - p.position) * p.speed;
        float turn = std::min(1.f, dt * 8 * p.turnMultiplier);
        p.velocity = p.velocity * (1 - turn) + desired * turn;
        p.velocity = Unit(p.velocity) * p.speed;
      }
    }
    p.position = p.position + p.velocity * dt;
    if(!p.hostile && p.element==Element::Water) {
      for(auto &fire:state_.effects) if(fire.hostile && fire.kind==EffectKind::Burn &&
          SegmentDistance(fire.position,p.previous,p.position)<=fire.radius+p.radius) fire.life=0;
    }
    p.life -= dt;
    if (p.pathBurn) {
      p.pathTick -= dt;
      if (p.pathTick <= 0) {
        p.pathTick = .15f;
        EffectState trail;
        trail.position = p.position;
        trail.kind = EffectKind::Burn;
        trail.radius = .5f;
        trail.life = 2;
        trail.interval = .5f;
        trail.damage = 1;
        trail.oneShot = false;
        trail.element = Element::Fire;
        trail.form = Form::Sword;
        trail.source = p.source;
        AddEffect(trail);
      }
    }
    if (p.hostile) {
      bool hit = SegmentDistance(state_.player.position, p.previous,
                                 p.position) < p.radius + .4f;
      if (hit) {
        Hurt(p.damage, DamageKind::Projectile, p.owner, p.position);
        p.life = 0;
      }
      if (p.life <= 0) {
        EffectState fx;
        fx.kind = EffectKind::Burn;
        fx.position = p.position;
        fx.radius = 1;
        fx.life = 4;
        fx.damage = 1.5f;
        fx.interval = .5f;
        fx.oneShot = false;
        fx.hostile = true;
        AddEffect(fx);
      }
    } else {
      for (auto &e : state_.enemies) {
        if (p.pierce <= 0 || e.hp <= 0 ||
            std::find(p.hit.begin(), p.hit.end(), e.id) != p.hit.end())
          continue;
        if (SegmentDistance(e.position, p.previous, p.position) <
            p.radius + e.radius) {
          p.hit.push_back(e.id);
          Hit(e, p.damage, p.element, p.form, true, p.burnDps, p.burnDuration);
          if (p.chains)
            Chain(e.id, p.chains, p.damage, p.chainScale, 3, p.form);
          if (--p.pierce <= 0) {
            // Exhausting penetration ends the outbound leg too; the return
            // branch below resets its hit list and penetration budget.
            p.life = 0;
            break;
          }
        }
      }
      if (p.life <= 0 && p.returning && !p.returned) {
        p.returned = true;
        // Do not steer a returning sword back toward its outbound target.
        p.homing = false;
        p.hit.clear();
        p.pierce = 6;
        p.velocity = Unit(state_.player.position - p.position) * p.speed;
        p.life = Len(state_.player.position - p.position) / p.speed + .1f;
      }
    }
    if (std::abs(p.position.x) > 50 || std::abs(p.position.y) > 29)
      p.life = 0;
  }
  state_.projectiles.erase(
      std::remove_if(state_.projectiles.begin(), state_.projectiles.end(),
                     [](const auto &p) { return p.life <= 0; }),
      state_.projectiles.end());
}
void Simulation::UpdateEffects(float dt) {
  const std::size_t n = state_.effects.size();
  for (std::size_t i = 0; i < n; ++i) {
    auto &e = state_.effects[i];
    if(e.life<=0)continue;
    currentSource_ = e.source;
    if (e.followPlayer) {
      e.position = state_.player.position;
      if (e.form == Form::Beast) {
        float angle=state_.statistics.elapsed*1.7f + float(e.id);
        e.position=Constrain(e.position+Vec2{std::cos(angle),std::sin(angle)}*2.2f,.2f);
      }
    }
    if (!e.hostile && e.element == Element::Water && e.warning<=0) {
      for(auto &fire:state_.effects) if(fire.hostile && fire.kind==EffectKind::Burn &&
          Len2(fire.position-e.position)<=(e.radius+fire.radius)*(e.radius+fire.radius)) fire.life=0;
    }
    if (e.owner && e.hostile) {
      auto *owner = Find(e.owner);
      if (!owner || owner->hp <= 0) {
        e.life = 0;
        continue;
      }
      if (owner->stun > 0 && e.warning > 0)
        continue;
    }
    if (e.warning > 0) {
      Down(e.warning, dt);
      continue;
    }
    e.life -= dt;
    e.tick -= dt;
    if (e.damage <= 0 || e.tick > 0)
      continue;
    e.tick = std::max(.05f, e.interval);
    if (!e.hostile && e.form==Form::Beast) {
      int rank=1;for(auto spell:state_.progression.build.spells)if(spell.id==e.source)rank=spell.rank;
      const auto spec=GetSpellConfig(e.source,rank);
      EnemyState *target=nullptr;float score=1e9f;
      for(auto &enemy:state_.enemies) {
        float distance=Len2(enemy.position-e.position);
        float candidate=distance*(enemy.role==EnemyRole::Ranged||enemy.role==EnemyRole::Exploder?.4f:1.f);
        if(enemy.hp>0 && distance<=spec.radius*spec.radius && candidate<score){target=&enemy;score=candidate;}
      }
      if(target) {
        ProjectileState shot;shot.position=e.position;shot.previous=e.position;shot.target=target->id;
        shot.source=e.source;shot.form=e.form;shot.element=e.element;shot.damage=e.damage;
        shot.homing=true;shot.velocity=Unit(target->position-e.position)*14;shot.speed=14;
        shot.life=std::max(1.f,spec.radius/14+.5f);shot.chains=spec.chains;shot.chainScale=spec.chainScale;
        shot.burnDps=e.burnDps;shot.burnDuration=e.burnDuration;AddProjectile(shot);
      }
      continue;
    }
    if (e.hostile) {
      bool inside = false;
      if (e.kind == EffectKind::FireWall) {
        float elapsed = e.duration - e.life;
        float x = 48 - elapsed * 5, y = 27 - elapsed * 2.5f;
        inside = std::abs(state_.player.position.x) > x - 1 ||
                 std::abs(state_.player.position.y) > y - 1;
        e.radius = x;
      } else
        inside = Len2(state_.player.position - e.position) <
                 (e.radius + .4f) * (e.radius + .4f);
      if (inside)
        Hurt(e.damage, DamageKind::HostileArea, e.owner, e.position, e.kind);
    } else
      Area(e.position, e.radius, e.damage, e.element, e.form, true, e.burnDps,
           e.burnDuration, e.stun);
    if (e.oneShot)
      e.damage = 0;
  }
  state_.effects.erase(
      std::remove_if(state_.effects.begin(), state_.effects.end(),
                     [](const auto &e) { return e.life <= 0; }),
      state_.effects.end());
}
void Simulation::ResolveDeaths() {
  // Remove before death procs so secondary explosions can never re-kill an
  // entity.
  bool bossDefeated = false;
  for (;;) {
    std::vector<EnemyState> dead;
    for (const auto &e : state_.enemies)
      if (e.hp <= 0)
        dead.push_back(e);
    if (dead.empty())
      break;
    state_.enemies.erase(
        std::remove_if(state_.enemies.begin(), state_.enemies.end(),
                       [](const auto &e) { return e.hp <= 0; }),
        state_.enemies.end());
    for (auto &e : dead) {
      if (e.role == EnemyRole::Boss) {
        if (state_.phase == RunPhase::Dead)
          continue;
        if (e.bossPhase == 1) {
          BossPhaseTwo(e);
          state_.enemies.push_back(e);
        } else {
          bossDefeated = true;
        }
        continue;
      }
      ++state_.statistics.kills;
      if (fullGame_) {
        campaign_.RecordKills(1);
      }
      if (progression_.HasTrigger(1))
        speedBuff_ = 2;
      if (progression_.HasTrigger(4) && state_.statistics.kills % 25 == 0)
        state_.player.hp = std::min(
            state_.progression.build.stats.maxHp,
            state_.player.hp + state_.progression.build.stats.maxHp * .05f);
      float xp[] = {1, 1, 4, 4, 6, 8, 0, 4};
      PickupState pickup;
      pickup.id = Id();
      pickup.position = e.position;
      if(fullGame_ && HasFeature(campaign_.GetState().tier,Feature::Economy)) {
        float stones=EconomyConfig{}.killStones[int(e.tier)]*anomalies_.Modifiers(state_.player.position).currency;
        pickup.spiritStones=int(stones);
        if(Random()<stones-pickup.spiritStones)++pickup.spiritStones;
      }
      pickup.xp = (e.tier == EnemyTier::Elite ? 25 : xp[int(e.role)]) * (fullGame_ ? anomalies_.Modifiers(state_.player.position).xp : 1);
      auto relic =
          progression_.OnKill(e.tier, e.role == EnemyRole::SpiritBeast);
      if (relic) {
        pickup.hasRelic = true;
        pickup.relic = *relic;
      }
      if (state_.pickups.size() < config_.maxPickups)
        state_.pickups.push_back(pickup);
      else {
        progression_.AddXp(pickup.xp);
        if(fullGame_)economy_.CreditStones(pickup.spiritStones);
        if (pickup.hasRelic)
          ReceiveRelic(pickup.relic);
        ++state_.statistics.droppedEvents;
      }
      if (e.burnRemaining > 0) {
        if (state_.progression.build.resonance.fire >= 4)
          Area(e.position, 2, 20, Element::None, Form::None, false);
        if (progression_.HasTrigger(6) && Random() < .1f)
          Area(e.position, 1.5f, 12, Element::None, Form::None, false);
      }
      if (e.mutations & 4)
        for (int k = 0; k < 2; ++k) {
          EntityId id = Spawn(e.role, e.position + Vec2{float(k * 2 - 1), 0},
                              EnemyTier::Variant);
          if (auto *child = Find(id)) {
            child->hp = child->maxHp = e.maxHp * .4f;
            child->mutations = 0;
          }
        }
    }
  }
  if (fullGame_ && state_.phase != RunPhase::Dead && opportunityEnemy_ && state_.enemies.empty()) {
    opportunityEnemy_ = 0; campaign_.CompleteOpportunity();
    state_.phase = RunPhase::Rest; CollectPickups(true); progression_.BeginRest();
    economy_.BeginRest(progression_, ++restSerial_, false);
    state_.effects.clear(); state_.projectiles.clear(); state_.enemies.clear();
  }
  if (bossDefeated) {
    if (fullGame_) campaign_.CompleteBoss();
    state_.phase = fullGame_ && campaign_.GetState().mode == GameMode::Endless ? RunPhase::Rest : RunPhase::Won;
    state_.bossPhase = 2;
    state_.projectiles.clear();
    state_.effects.clear();
    state_.enemies.clear();
    CollectPickups(true);
    progression_.BeginRest();
    if(fullGame_) progression_.GenerateBossRewards(std::uint64_t(campaign_.GetState().bosses));
    else progression_.GenerateBossRewards();
    if (fullGame_) {
      if (state_.phase == RunPhase::Won) economy_.EndRun();
      economy_.BeginRest(progression_, ++restSerial_, false);
    }
  }
  Sync();
}
void Simulation::CollectPickups(bool all) {
  float radius = state_.progression.build.stats.pickupRadius;
  for (auto &p : state_.pickups)
    if (all || Len2(p.position - state_.player.position) <= radius * radius) {
      progression_.AddXp(p.xp);
      if(fullGame_)economy_.CreditStones(p.spiritStones);
      if (p.hasRelic)
        ReceiveRelic(p.relic);
      p.id = 0;
    }
  state_.pickups.erase(std::remove_if(state_.pickups.begin(),
                                      state_.pickups.end(),
                                      [](const auto &p) { return p.id == 0; }),
                       state_.pickups.end());
  Sync();
}
void Simulation::EndWave() {
  beastPending_ = false;
  anomalyWarningShown_=false;
  CollectPickups(true);
  state_.enemies.clear();
  state_.projectiles.clear();
  state_.effects.clear();
  state_.phase = RunPhase::Rest;
  if (fullGame_) campaign_.CompleteWave();
  progression_.OnWaveEnded();
  progression_.BeginRest();
  if (fullGame_) economy_.BeginRest(progression_, ++restSerial_, false);
  Sync();
}
void Simulation::UpdateCaveBoss(EnemyState &boss, float dt) {
  const auto r=BossRules();const int phase=std::clamp(boss.bossPhase-1,0,1);
  Vec2 direction=Unit(state_.player.position-boss.position);
  if(Len2(state_.player.position-boss.position)>36)
    boss.position=Steer(boss.position,direction,boss.speed*(1-boss.slowFraction)*dt,boss.radius);
  bossTimer_+=dt;bossSummon_+=dt;wallTimer_+=dt;
  auto hazard=[&](Vec2 point, bool roots) {
    EffectState effect;effect.owner=boss.id;effect.hostile=true;
    effect.kind=roots?EffectKind::Circle:EffectKind::Burn;
    effect.element=roots?Element::Wood:Element::Fire;effect.form=roots?Form::Array:Form::None;
    effect.position=Constrain(point,.1f);effect.radius=roots?r.rootRadius:r.flameRadius;
    effect.warning=roots?r.rootWarning:r.flameWarning;
    effect.life=roots?r.rootDuration:.3f;effect.interval=roots?r.rootTick:1;
    effect.damage=roots?r.rootDamage[phase]:r.flameDamage[phase];effect.oneShot=!roots;
    // The effect itself owns both warning and damage; a full cap drops both safely.
    AddEffect(effect);
  };
  if(arena_.cave==CaveId::EmberMountain) {
    if(bossTimer_>=r.flamePeriod[phase]) {
      bossTimer_-=r.flamePeriod[phase];Vec2 side{-direction.y,direction.x};
      for(int k=-1;k<=1;++k)hazard(boss.position+direction*5+side*(float(k)*3.5f),false);
    }
    if(phase==1 && wallTimer_>=r.ringPeriod) {
      wallTimer_-=r.ringPeriod;
      for(int k=0;k<8;++k){float angle=k*Pi/4;hazard(boss.position+Vec2{std::cos(angle),std::sin(angle)}*r.ringRadius,false);}
    }
    if(bossSummon_>=r.flameSummon[phase]) {
      bossSummon_-=r.flameSummon[phase];
      for(int k=0;k<(phase?4:2);++k)Spawn(phase?(k<2?EnemyRole::Ranged:EnemyRole::Charger):EnemyRole::Exploder,Edge(),EnemyTier::Variant);
    }
  } else {
    if(bossTimer_>=r.rootPeriod[phase]) {
      bossTimer_-=r.rootPeriod[phase];hazard(state_.player.position,true);
      if(phase)hazard(state_.player.position+Vec2{-direction.y,direction.x}*4,true);
    }
    if(bossSummon_>=r.mistSummon[phase]) {
      bossSummon_-=r.mistSummon[phase];
      if(phase){Spawn(EnemyRole::Support,Edge(),EnemyTier::Variant);Spawn(EnemyRole::Summoner,Edge(),EnemyTier::Variant);}
      else {Spawn(EnemyRole::Support,Edge(),EnemyTier::Variant);for(int k=0;k<2;++k)Spawn(EnemyRole::Swarm,Edge(),EnemyTier::Normal);}
    }
  }
}
void Simulation::StartBoss() {
  state_.phase = RunPhase::Boss;
  state_.bossPhase = 1;
  state_.waveElapsed = 0;
  bossTimer_ = bossSummon_ = bossShield_ = wallTimer_ = chargeTimer_ = 0;
  Spawn(EnemyRole::Boss, {0, -12}, EnemyTier::Boss);
  for (int k = 0; k < 2; ++k) {
    EnemyRole role=EnemyRole::Support;
    if(fullGame_ && arena_.cave==CaveId::EmberMountain)role=EnemyRole::Exploder;
    if(fullGame_ && arena_.cave==CaveId::MistForest && k==1)role=EnemyRole::Summoner;
    Spawn(role,{float(k*12-6),-16},EnemyTier::Variant);
  }
}
void Simulation::BossPhaseTwo(EnemyState &e) {
  e.hp = e.maxHp = config_.bossHp2 * (fullGame_ && campaign_.GetState().mode==GameMode::Endless ? ScheduleWave(GameMode::Endless,state_.wave,campaign_.GetConfig()).healthScale : 1);
  e.bossPhase = 2;
  e.windup = e.chargeRemaining = 0;
  state_.bossPhase = 2;
  bossTimer_ = bossSummon_ = bossShield_ = wallTimer_ = chargeTimer_ = 0;
  state_.projectiles.clear();
  state_.effects.clear();
  EffectState fx;
  fx.kind = EffectKind::Explosion;
  fx.position = e.position;
  fx.radius = 6;
  fx.warning = 1.2f;
  fx.life = .4f;
  AddEffect(fx);
  if(fullGame_ && arena_.cave!=CaveId::ThunderMarsh) {
    // ResolveDeaths temporarily removed the boss; reserve its reinsertion slot.
    for(int k=0;k<6 && state_.enemies.size()+1<std::min<std::size_t>(80,config_.maxEnemies);++k) {
      EnemyRole role=arena_.cave==CaveId::EmberMountain ? (k<4?EnemyRole::Exploder:EnemyRole::Ranged) :
          (k==0?EnemyRole::Support:k==1?EnemyRole::Summoner:EnemyRole::Swarm);
      Spawn(role,Edge(),EnemyTier::Variant);
    }
    return;
  }
  for (int k = 0; k < 10 && state_.enemies.size() + 1 <
                                std::min<std::size_t>(80, config_.maxEnemies);
       ++k)
    Spawn(k < 4 ? EnemyRole::Ranged : EnemyRole::Charger, Edge(),
          EnemyTier::Variant);
}
} // namespace dywx::mvp
