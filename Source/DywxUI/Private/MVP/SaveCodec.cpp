#include "MVP/SaveCodec.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>
#include <type_traits>
namespace dywx::mvp {
// Explicit field encoding, never native object layout/padding or pointers.
class SaveArchive {
public:
  bool reading=false, ok=true;
  std::string bytes;
  std::size_t offset=0;
  SaveArchive()=default;
  explicit SaveArchive(const std::string& input):reading(true),bytes(input){}
  template<class... T> void operator()(T&... v){(Process(v),...);}
  std::size_t Count(std::size_t count, std::size_t limit=1000000) {
    std::uint64_t n=count;Process(n);
    if(n>limit||(reading&&n>(bytes.size()-std::min(offset,bytes.size()))/8)) {ok=false;return 0;}
    return static_cast<std::size_t>(n);
  }
  template<class T> void Process(T& v) {
    if(!ok)return;
    if constexpr(std::is_enum_v<T>){int n=int(v);Process(n);if(reading)v=static_cast<T>(n);}
    else if constexpr(std::is_integral_v<T>) {
      std::uint64_t u=0;
      if(!reading)u=static_cast<std::uint64_t>(v);
      Number(u);
      if(reading&&ok) {
        if constexpr(std::is_same_v<T,bool>){if(u>1){ok=false;return;}v=u!=0;}
        else if constexpr(std::is_signed_v<T>){std::int64_t x=0;std::memcpy(&x,&u,8);if(x<std::numeric_limits<T>::min()||x>std::numeric_limits<T>::max()){ok=false;return;}v=static_cast<T>(x);}
        else {if(u>std::numeric_limits<T>::max()){ok=false;return;}v=static_cast<T>(u);}
      }
    } else if constexpr(std::is_floating_point_v<T>) {
      double d=v;std::uint64_t u=0;static_assert(sizeof(double)==8&&std::numeric_limits<double>::is_iec559);
      if(!reading)std::memcpy(&u,&d,8);
      Number(u);if(reading)std::memcpy(&d,&u,8);
      if(!std::isfinite(d)||std::abs(d)>std::numeric_limits<T>::max()){ok=false;return;}if(reading)v=static_cast<T>(d);
    } else Fields(*this,v);
  }
  void Process(std::string& s){
    std::uint64_t n=s.size();Process(n);if(!ok||n>SaveCodec::MaxBytes){ok=false;return;}
    if(reading){if(n>bytes.size()-offset){ok=false;return;}s.assign(bytes.data()+offset,static_cast<std::size_t>(n));offset+=static_cast<std::size_t>(n);}
    else {if(bytes.size()+n>SaveCodec::MaxBytes){ok=false;return;}bytes+=s;}
  }
  template<class T,std::size_t N>void Process(std::array<T,N>& v){for(auto& x:v)Process(x);}
  template<class T>void Process(std::vector<T>& v){auto n=Count(v.size());if(reading)v.resize(n);for(auto& x:v)Process(x);}
  template<class K,class V>void Process(std::map<K,V>& m){auto n=Count(m.size());if(reading){m.clear();for(std::size_t i=0;i<n&&ok;++i){K k{};V v{};(*this)(k,v);if(!m.emplace(k,std::move(v)).second)ok=false;}}else for(auto& kv:m){auto k=kv.first;(*this)(k,kv.second);}}
  template<class K>void Process(std::set<K>& s){auto n=Count(s.size());if(reading){s.clear();for(std::size_t i=0;i<n&&ok;++i){K k{};Process(k);if(!s.insert(k).second)ok=false;}}else for(auto k:s)Process(k);}
  template<class T>void Process(std::optional<T>& v){bool has=v.has_value();Process(has);if(reading){if(has)v=T{};else v.reset();}if(has&&ok)Process(*v);}
private:
  void Number(std::uint64_t& u){
    if(reading){if(offset>bytes.size()||bytes.size()-offset<8){ok=false;return;}u=0;for(unsigned i=0;i<8;++i)u|=std::uint64_t(static_cast<unsigned char>(bytes[offset++]))<<(8*i);}
    else {if(bytes.size()+8>SaveCodec::MaxBytes){ok=false;return;}for(unsigned i=0;i<8;++i)bytes.push_back(char((u>>(8*i))&255));}
  }
};
#define F(T,...) template<class A> void Fields(A& a,T& v){a(__VA_ARGS__);}
F(Vec2,v.x,v.y)
F(Stats,v.maxHp,v.armor,v.regen,v.power,v.cooldownReduction,v.area,v.critChance,v.critMultiplier,v.moveSpeed,v.pickupRadius,v.dashCooldown,v.luck,v.xpMultiplier,v.burnMultiplier,v.chainMultiplier,v.triggers,v.mysteries,v.uniques,v.elementBonus,v.formBonus)
F(SpellState,v.id,v.rank)
F(Resonance,v.fire,v.thunder,v.body,v.sword,v.array,v.metal,v.wood,v.water,v.earth,v.beast,v.talisman)
F(BuildState,v.hero,v.spells,v.passives,v.stats,v.resonance)
F(Affix,v.kind,v.value,v.code)
F(Relic,v.id,v.catalogId,v.rarity,v.affixes,v.locked,v.carryOut)
F(UpgradeOffer,v.kind,v.spell,v.passive)
F(ProgressionState,v.build,v.xp,v.level,v.pendingUpgrades,v.rerollsRemaining,v.xpRequired,v.offers,v.equipped,v.bag,v.pendingRefineXp,v.pityWaves,v.systemLayer,v.sealsRemaining,v.skippedUpgrades,v.skipSpiritStones,v.sealedSpells,v.sealedPassives,v.bossRewards,v.seenSpells,v.seenRelics,v.seenFusions,v.seenPassives,v.seenEnemies,v.seenMutations,v.seenResonances)
F(EconomyConfig,v.killStones,v.refineStones,v.shopPrices,v.refreshPrice,v.reforgeBase,v.reforgeStep,v.treasureBase,v.treasureStep,v.paidTreasureCap,v.traditionalGoods,v.pityWaves,v.treasureWeights,v.rarityWeight)
F(ShopGood,v.token,v.relic,v.price,v.sold)
F(ReforgeOffer,v.relic,v.index,v.oldAffix,v.newAffix,v.paid)
F(EconomyUndo,v.relics,v.amount,v.reward)
F(EconomyState,v.version,v.config,v.rng,v.quick,v.layer2,v.layer3,v.active,v.cave,v.freeRefresh,v.restId,v.token,v.stones,v.pendingXp,v.paidTreasure,v.shop,v.hidden,v.reforges,v.reforge,v.treasure,v.freeTreasures,v.undo)
F(HeroRecord,v.hero,v.bestWave,v.kills,v.runs,v.wins,v.mastery)
F(EndlessRecord,v.runId,v.hero,v.wave,v.kills)
F(CampaignProfile,v.daoYun,v.anyWin,v.traditionalWin,v.unlockedRisk,v.unlocks,v.bonusRanks,v.heroes,v.nextRunId,v.settledThrough,v.seenAnomalies,v.seenBosses,v.endlessRanking)
F(CampaignConfig,v.quickSixWaves,v.traditionalWaveSeconds,v.endlessWaveSeconds,v.riskRewardPerPoint,v.endlessBossReward,v.killsPerDaoYun,v.legacyUnlockCost,v.permanentRankCost,v.eventStones,v.eventXp)
F(CampaignRunState,v.runId,v.rng,v.kills,v.mode,v.hero,v.wave,v.bosses,v.riskMask,v.tier,v.phase,v.offers,v.offerCount,v.selected,v.caveAbsent,v.chestAbsent,v.opportunityResolved,v.settled)
F(RunConfig,v.sixWaves,v.formResonance,v.waveDurations,v.fixedStep,v.spawnGrace,v.maxEnemies,v.maxProjectiles,v.maxEffects,v.maxPickups,v.bossHp1,v.bossHp2,v.spawning,v.automaticSpells)
F(PlayerState,v.position,v.facing,v.hp,v.shield,v.invulnerable,v.dashRemaining,v.dashCooldown)
F(RunStatistics,v.kills,v.shots,v.hits,v.dodges,v.damageDealt,v.damageTaken,v.elapsed,v.droppedEvents)
F(ArenaObstacle,v.position,v.radius)
F(ArenaConfig,v.halfWidth,v.halfHeight,v.cornerCut,v.centralClearRadius,v.gridSize,v.navigationClearance,v.peripheralMargin,v.obstacleCount,v.placementAttempts,v.veinCount,v.minObstacleRadius,v.maxObstacleRadius,v.maxObstacleFraction)
F(ArenaLayout,v.seed,v.cave,v.config,v.obstacles,v.veins,v.hazards,v.beastRoute,v.edgeSpawns,v.usedFallback)
F(AnomalyConfig,v.interval,v.activeDuration,v.warning,v.pulseInterval,v.hazardRadius,v.lightningDamage,v.fissureDamage,v.thunderReward,v.thunderRewardDuration,v.tideSpawn,v.tideCurrency,v.fissureChargers,v.fogVision,v.fogRanged,v.fogBeasts,v.eclipseEnemyPower,v.eclipseXp,v.arrayPower,v.arrayRadius)
F(AnomalyState,v.enabled,v.active,v.id,v.anchor,v.elapsed,v.remaining,v.warningRemaining,v.encountered)
F(CombatBoundaryState,v.procCooldowns,v.speedBuff,v.powerBuff,v.shieldTime,v.bodyProcCooldown,v.critCooldown,v.resetTimer,v.terrainTimer,v.lowHpUsed,v.lowHpActive)
F(ProfileData,v.known,v.campaign,v.checkpoints)
#undef F
namespace {
template<class E>bool Enum(E e,int count){return int(e)>=0&&int(e)<count;}
bool AffixValid(const Affix& a){return Enum(a.kind,int(AffixKind::Count))&&std::isfinite(a.value)&&std::abs(a.value)<=1000000&&a.code>=0&&a.code<=100;}
bool RelicValid(const Relic& r){return r.id&&r.id<std::numeric_limits<EntityId>::max()&&r.catalogId>=0&&r.catalogId<int(kRelicCount)&&Enum(r.rarity,4)&&r.affixes.size()<=64&&std::all_of(r.affixes.begin(),r.affixes.end(),AffixValid);}
std::uint64_t Hash(const std::string& s){std::uint64_t h=1469598103934665603ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
bool Wrap(const std::string& payload,std::uint64_t type,std::string& out){SaveArchive a;std::uint64_t magic=0x4459575853415645ull,version=2,hash=Hash(payload);auto p=payload;a(magic,version,type,hash,p);if(!a.ok)return false;out=std::move(a.bytes);return true;}
bool Unwrap(const std::string& bytes,std::uint64_t type,std::string& out){if(bytes.size()>SaveCodec::MaxBytes)return false;SaveArchive a(bytes);std::uint64_t magic=0,version=0,actual=0,hash=0;std::string p;a(magic,version,actual,hash,p);if(!a.ok||a.offset!=bytes.size()||magic!=0x4459575853415645ull||version!=2||actual!=type||hash!=Hash(p))return false;out=std::move(p);return true;}
bool ProfileValid(const ProfileData& d){
  const auto& p=d.campaign;if(!p.nextRunId||p.unlockedRisk>4||p.heroes.size()>3||(p.traditionalWin&&!p.anyWin))return false;
  unsigned ranks=0;for(auto rank:p.bonusRanks){if(rank>10)return false;ranks+=rank;}if(ranks>10)return false;
  std::set<int> heroes;for(const auto& h:p.heroes)if(h.hero<0||h.hero>2||h.bestWave<0||h.bestWave>1000000||h.wins>h.runs||!heroes.insert(h.hero).second)return false;
  for(auto n:p.settledThrough)if(n>=p.nextRunId)return false;
  if(p.endlessRanking.size()>20)return false;
  std::set<std::uint64_t> rankedIds;
  auto rankBefore=[](const EndlessRecord& a,const EndlessRecord& b){
    if(a.wave!=b.wave)return a.wave>b.wave;
    if(a.kills!=b.kills)return a.kills>b.kills;
    if(a.runId!=b.runId)return a.runId<b.runId;
    return a.hero<b.hero;
  };
  for(std::size_t i=0;i<p.endlessRanking.size();++i){
    const auto& r=p.endlessRanking[i];
    if(r.hero<0||r.hero>2||r.wave<1||!r.runId||r.runId>=p.nextRunId||r.runId>p.settledThrough[std::size_t(r.hero)]||
       !rankedIds.insert(r.runId).second||(i&&rankBefore(r,p.endlessRanking[i-1])))return false;
  }
  if(d.known.pityWaves<0||d.known.pityWaves>1000000)return false;
  for(std::size_t i=0;i<d.checkpoints.size();++i)if(!d.checkpoints[i].empty()){
    RunCheckpoint cp;if(!SaveCodec::DecodeCheckpoint(d.checkpoints[i],cp)||cp.campaign.hero!=int(i)||cp.campaign.runId>=p.nextRunId||cp.campaign.runId<=p.settledThrough[i])return false;
  }
  return true;
}
}
void SaveCodec::ProgressionFields(SaveArchive& a,Progression& p){
  std::string rng;if(!a.reading){std::ostringstream ss;ss.imbue(std::locale::classic());ss<<p.rng_;rng=ss.str();}
  a(p.state_,rng,p.fullGame_,p.permanentBase_,p.dropRiskWeight_,p.spellPoolUnlocked_,p.relicPoolUnlocked_,p.preferredRelics_,p.lastBossOrdinal_,p.nextRelic_,p.formResonance_,p.resting_,p.immortalThisWave_,p.bossRewardsGenerated_,p.pityWaves_,p.pendingRefineXp_,p.consumed_);
  auto n=a.Count(p.undo_.size(),100000);if(a.reading)p.undo_.resize(n);for(auto& u:p.undo_)a(u.relics,u.xp);
  if(a.reading&&a.ok){std::istringstream ss(rng);ss.imbue(std::locale::classic());if(rng.size()>20000||!(ss>>p.rng_)){a.ok=false;return;}ss>>std::ws;if(!ss.eof())a.ok=false;}
}
void SaveCodec::AnomalyFields(SaveArchive& a,AnomalySystem& s){
  a(s.layout_,s.config_,s.state_,s.order_,s.cycle_,s.clock_,s.nextStart_,s.nextPulse_,s.strikeAt_,s.endAt_,s.thunderUntil_);
  if(s.cycle_>100000000||s.clock_<0||s.nextStart_<0||s.nextPulse_<0||s.strikeAt_<-1||s.endAt_<0||s.thunderUntil_<0||!Enum(s.state_.id,6))a.ok=false;
  const auto& c=s.config_;
  if(c.interval<2||c.interval>3600||c.activeDuration<1||c.activeDuration>c.interval||c.warning<.1f||c.warning>c.activeDuration||
     c.pulseInterval<c.warning+.1f||c.pulseInterval>3600||c.hazardRadius<=0||c.hazardRadius>10||c.arrayRadius<=0||c.arrayRadius>10||
     s.state_.elapsed<0||s.state_.remaining<0||s.state_.warningRemaining<0||
     (s.state_.active?s.endAt_<s.clock_:s.nextStart_<s.clock_)||
     (s.strikeAt_>=0&&s.strikeAt_<s.clock_))a.ok=false;
  std::set<int> unique;for(auto id:s.order_)if(!Enum(id,6)||!unique.insert(int(id)).second)a.ok=false;
}
bool SaveCodec::ValidateProgression(const Progression& p){
  const auto& s=p.state_;
  if(!Enum(s.build.hero,3)||s.build.spells.size()>6||s.build.passives.size()>4||s.equipped.size()>6||s.offers.size()>3||s.bossRewards.size()>3||
     s.xp<0||s.xpRequired<=0||s.level<1||s.level>1000000||s.pendingUpgrades<0||s.pendingUpgrades>1000000||s.rerollsRemaining<0||s.rerollsRemaining>100||
     s.systemLayer<1||s.systemLayer>3||s.sealsRemaining<0||s.sealsRemaining>3||s.skippedUpgrades<0||s.skipSpiritStones<0||
     p.pityWaves_<0||p.pityWaves_>1000000||s.pityWaves!=p.pityWaves_||p.pendingRefineXp_<0||s.pendingRefineXp!=p.pendingRefineXp_||
     !p.nextRelic_||p.nextRelic_==std::numeric_limits<EntityId>::max()||p.dropRiskWeight_<1||p.dropRiskWeight_>100)return false;
  std::set<int> spells,passives;for(const auto& x:s.build.spells)if(!Enum(x.id,int(kSpellCount))||x.rank<1||x.rank>5||!spells.insert(int(x.id)).second)return false;
  for(auto x:s.build.passives)if(!Enum(x,int(kPassiveCount))||!passives.insert(int(x)).second)return false;
  for(const auto& x:s.offers)if(!Enum(x.kind,4)||!Enum(x.spell,int(kSpellCount))||!Enum(x.passive,int(kPassiveCount)))return false;
  if((s.pendingUpgrades==0)!=s.offers.empty())return false;
  const auto legal=p.LegalOffers();std::set<std::pair<int,int>> offerIds;
  for(const auto& offer:s.offers){
    if(!offerIds.emplace(int(offer.kind),offer.kind==UpgradeKind::Passive?int(offer.passive):int(offer.spell)).second)return false;
    if(std::none_of(legal.begin(),legal.end(),[&](const UpgradeOffer& x){return x.kind==offer.kind&&x.spell==offer.spell&&x.passive==offer.passive;}))return false;
  }
  std::set<EntityId> ids;for(const auto* v:{&s.equipped,&s.bag,&s.bossRewards})for(const auto& r:*v)if(!RelicValid(r)||!ids.insert(r.id).second||r.id>=p.nextRelic_)return false;
  std::int64_t xp=0;for(const auto& u:p.undo_){if(u.xp<0)return false;xp+=u.xp;for(const auto& r:u.relics)if(!RelicValid(r)||!ids.insert(r.id).second||r.id>=p.nextRelic_)return false;}
  if(xp!=p.pendingRefineXp_)return false;
  for(int id:p.preferredRelics_)if(id<0||id>=int(kRelicCount))return false;
  return true;
}
bool SaveCodec::ValidateCheckpoint(const RunCheckpoint& c){
  // Public RestoreCheckpoint also accepts an object, so reject NaN/oversize values here,
  // not only at the byte decoder boundary.
  auto finite=c;SaveArchive fields;auto economy=finite.economy.ExportState();
  ProgressionFields(fields,finite.progression);
  fields(economy,finite.campaign,finite.campaignConfig,finite.runConfig,finite.player,finite.statistics,finite.arena,finite.combat,finite.spellCooldowns,finite.casts);
  AnomalyFields(fields,finite.anomalies);if(!fields.ok)return false;
  Campaign campaign(c.campaignConfig);
  if(c.campaign.mode==GameMode::Quick||c.campaign.settled||!campaign.Restore(c.campaign)||
     (c.campaign.phase!=CampaignPhase::Wave&&c.campaign.phase!=CampaignPhase::Boss)||
     c.campaign.hero!=int(c.progression.GetState().build.hero)||!ValidateProgression(c.progression)||
     c.progression.IsResting()||!c.progression.GetState().bossRewards.empty()||
     !c.seed||!c.simulationRng||!c.nextEntityId||c.nextEntityId==std::numeric_limits<EntityId>::max()||c.realm<0||c.realm>2||c.player.hp<=0||c.player.hp>100000000||
     c.runConfig.fixedStep<=0||c.runConfig.fixedStep>1||c.runConfig.spawnGrace<0||
     c.runConfig.maxEnemies==0||c.runConfig.maxEnemies>100000||c.runConfig.maxEffects>100000||c.runConfig.maxProjectiles>100000||c.runConfig.maxPickups>100000||
     c.runConfig.bossHp1<=0||c.runConfig.bossHp2<=0||c.campaignConfig.traditionalWaveSeconds<=0||c.campaignConfig.endlessWaveSeconds<=0||
     c.campaignConfig.riskRewardPerPoint<0||c.realm!=int(c.arena.cave))return false;
  for(auto n:c.spellCooldowns)if(n<0||n>1000000)return false;
  for(auto n:c.casts)if(n<0)return false;
  for(auto n:c.runConfig.waveDurations)if(n<=0||n>1000000)return false;
  // Avoid untrusted dimensions triggering enormous navigation grids.
  const auto& ac=c.arena.config;
  if(ac.halfWidth<=0||ac.halfWidth>256||ac.halfHeight<=0||ac.halfHeight>256||ac.gridSize<.25f||ac.gridSize>100||(4.*ac.halfWidth*ac.halfHeight/(ac.gridSize*ac.gridSize)>1000000)||
     c.arena.obstacles.size()>1000||c.arena.veins.size()>1000||c.arena.hazards.size()>1000||c.arena.edgeSpawns.size()>1000||!ValidateArena(c.arena))return false;
  SaveArchive outerArena,innerArena;auto outer=c.arena;auto inner=c.anomalies.layout_;outerArena(outer);innerArena(inner);
  if(outerArena.bytes!=innerArena.bytes)return false;
  Economy check;if(!check.RestoreState(c.economy.ExportState(),c.progression))return false;
  auto es=c.economy.ExportState();if(es.layer2!=(c.campaign.tier>=2)||es.layer3!=(c.campaign.tier>=3)||c.progression.state_.systemLayer!=int(c.campaign.tier)||es.active||es.quick||es.pendingXp||es.reforge||es.treasure||!es.undo.empty())return false;
  SaveArchive a;auto copy=c;AnomalyFields(a,copy.anomalies);return a.ok;
}
bool SaveCodec::EncodeCheckpoint(const RunCheckpoint& input,std::string& out){
  if(!ValidateCheckpoint(input))return false;
  auto c=input;SaveArchive a;auto e=c.economy.ExportState();
  ProgressionFields(a,c.progression);a(e,c.campaign,c.campaignConfig,c.runConfig,c.seed,c.simulationRng,c.nextEntityId,c.restSerial,c.realm,c.player,c.statistics,c.arena,c.combat,c.spellCooldowns,c.casts);AnomalyFields(a,c.anomalies);
  return a.ok&&Wrap(a.bytes,1,out);
}
bool SaveCodec::DecodeCheckpoint(const std::string& bytes,RunCheckpoint& out){
  std::string payload;if(!Unwrap(bytes,1,payload))return false;SaveArchive a(payload);RunCheckpoint c;EconomyState e;
  ProgressionFields(a,c.progression);a(e,c.campaign,c.campaignConfig,c.runConfig,c.seed,c.simulationRng,c.nextEntityId,c.restSerial,c.realm,c.player,c.statistics,c.arena,c.combat,c.spellCooldowns,c.casts);AnomalyFields(a,c.anomalies);
  if(!a.ok||a.offset!=payload.size()||!c.economy.RestoreState(e,c.progression)||!ValidateCheckpoint(c))return false;
  c.progression.Recompute();out=std::move(c);return true;
}
bool SaveCodec::EncodeProfile(const ProfileData& input,std::string& out){if(!ProfileValid(input))return false;auto d=input;SaveArchive a;a(d);return a.ok&&Wrap(a.bytes,2,out);}
bool SaveCodec::DecodeProfile(const std::string& bytes,ProfileData& out){std::string payload;if(!Unwrap(bytes,2,payload))return false;SaveArchive a(payload);ProfileData d;a(d);if(!a.ok||a.offset!=payload.size()||!ProfileValid(d))return false;out=std::move(d);return true;}
bool ProfileStore::Commit(ProfileData next){
  // Settlement and invalidation of this hero's older checkpoint are one durable transaction.
  // Never silently erase malformed blobs: preserve the recovery material and reject the commit.
  for(std::size_t i=0;i<next.checkpoints.size();++i)if(!next.checkpoints[i].empty()){
    RunCheckpoint checkpoint;if(!SaveCodec::DecodeCheckpoint(next.checkpoints[i],checkpoint)||checkpoint.campaign.hero!=int(i))return false;
    if(checkpoint.campaign.runId<=next.campaign.settledThrough[i])next.checkpoints[i].clear();
  }
  std::string bytes;if(!SaveCodec::EncodeProfile(next,bytes))return false;
  if(bytes!=lastCommitted_&&(!writer_||!writer_(bytes)))return false;
  lastCommitted_=std::move(bytes);data_=std::move(next);dirty_=false;return true;
}
bool ProfileStore::Load(const std::string& bytes){ProfileData d;if(!SaveCodec::DecodeProfile(bytes,d))return false;data_=std::move(d);lastCommitted_=bytes;dirty_=false;return true;}
bool ProfileStore::ImportLegacy(const ProgressionState& known){ProfileData d;d.known=known;std::string test;if(!SaveCodec::EncodeProfile(d,test))return false;data_=std::move(d);lastCommitted_.clear();dirty_=true;return true;}
void ProfileStore::Observe(const ProgressionState& s){
  auto merge=[&](auto& dst,const auto& src){for(std::size_t i=0;i<dst.size();++i)if(src[i]&&!dst[i]){dst[i]=true;dirty_=true;}};
  if(data_.known.pityWaves!=s.pityWaves){data_.known.pityWaves=std::clamp(s.pityWaves,0,1000000);dirty_=true;}
  merge(data_.known.seenSpells,s.seenSpells);merge(data_.known.seenRelics,s.seenRelics);merge(data_.known.seenFusions,s.seenFusions);merge(data_.known.seenPassives,s.seenPassives);merge(data_.known.seenEnemies,s.seenEnemies);merge(data_.known.seenMutations,s.seenMutations);merge(data_.known.seenResonances,s.seenResonances);
}
bool ProfileStore::Save(){return !dirty_||Commit(data_);}
std::uint64_t ProfileStore::AllocateRunId(){auto next=data_;auto id=ReserveRunId(next.campaign);return id&&Commit(std::move(next))?id:0;}
bool ProfileStore::SaveCheckpoint(HeroId hero,const std::string& blob){
  if(!Enum(hero,3))return false;
  RunCheckpoint cp;if(!SaveCodec::DecodeCheckpoint(blob,cp)||cp.campaign.hero!=int(hero)||cp.campaign.runId>=data_.campaign.nextRunId||cp.campaign.runId<=data_.campaign.settledThrough[int(hero)])return false;
  auto next=data_;next.checkpoints[int(hero)]=blob;return Commit(std::move(next));
}
bool ProfileStore::GetCheckpoint(HeroId hero,std::string& out) const {if(!Enum(hero,3)||data_.checkpoints[int(hero)].empty())return false;RunCheckpoint cp;const auto& s=data_.checkpoints[int(hero)];if(!SaveCodec::DecodeCheckpoint(s,cp)||cp.campaign.hero!=int(hero)||cp.campaign.runId<=data_.campaign.settledThrough[int(hero)])return false;out=s;return true;}
bool ProfileStore::HasCheckpoint(HeroId hero) const {std::string unused;return GetCheckpoint(hero,unused);}
bool ProfileStore::ConsumeCheckpoint(HeroId hero,std::string& out){std::string blob;if(!GetCheckpoint(hero,blob))return false;auto next=data_;next.checkpoints[int(hero)].clear();if(!Commit(std::move(next)))return false;out=std::move(blob);return true;}
}
