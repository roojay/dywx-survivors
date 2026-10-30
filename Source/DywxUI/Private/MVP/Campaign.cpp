#include "MVP/Campaign.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace dywx::mvp {
namespace {
bool ValidMode(GameMode m) { return m >= GameMode::Quick && m <= GameMode::Endless; }
unsigned Bits(unsigned m) { unsigned n=0; for (;m;m>>=1) n+=m&1; return n; }
std::uint64_t Add(std::uint64_t a, std::uint64_t b) {
  return b>std::numeric_limits<std::uint64_t>::max()-a ? std::numeric_limits<std::uint64_t>::max() : a+b;
}
float Safe(float n,float fallback,float low,float high) { return std::isfinite(n)?std::clamp(n,low,high):fallback; }
}
std::uint64_t ReserveRunId(CampaignProfile &p) {
  if (!p.nextRunId || p.nextRunId==std::numeric_limits<std::uint64_t>::max()) return 0;
  return p.nextRunId++;
}
unsigned FeatureTier(const CampaignProfile &p) { return p.traditionalWin?3:p.anyWin?2:1; }
bool HasFeature(unsigned tier, Feature f) {
  const int v=static_cast<int>(f);
  if (v<0 || v>static_cast<int>(Feature::Endless) || tier<1 || tier>3) return false;
  return tier >= (f>=Feature::Reforge?3u:f>=Feature::FormResonance?2u:1u);
}
bool HasFeature(const CampaignProfile &p,Feature f) { return HasFeature(FeatureTier(p),f); }
bool IsHeroUnlocked(const CampaignProfile &p,int h) {
  // Both prototype heroes start available; third is an inheritance unlock.
  return h==0 || h==1 || (h==2 && p.unlocks[static_cast<unsigned>(LegacyUnlock::HeroBeast)]);
}
bool IsRealmUnlocked(const CampaignProfile &p,int r) {
  return r==0 || (r==1 && p.unlocks[static_cast<unsigned>(LegacyUnlock::RealmSecond)]) ||
    (r==2 && p.unlocks[static_cast<unsigned>(LegacyUnlock::RealmThird)]);
}
WaveSchedule ScheduleWave(GameMode mode,int wave,const CampaignConfig &c) {
  WaveSchedule s;
  const int quickCount=c.quickSixWaves?6:4;
  if (!ValidMode(mode) || wave<1 || (mode==GameMode::Quick && wave>quickCount) || (mode==GameMode::Traditional && wave>10)) return s;
  s.valid=true;
  s.bossAfter=mode==GameMode::Quick?wave==quickCount:mode==GameMode::Traditional?wave==10:wave%5==0;
  if(mode==GameMode::Quick) {
    static constexpr float seconds4[]{100,120,140,160},health4[]{1,1.5f,2.2f,3.2f},damage4[]{1,1.15f,1.3f,1.5f};
    static constexpr float seconds6[]{70,75,80,85,90,95},health6[]{1,1.3f,1.7f,2.2f,2.7f,3.2f};
    static constexpr int caps4[]{40,70,100,130},caps6[]{30,50,70,90,110,130},elites[]{0,0,1,2};
    static constexpr std::array<int,6> counts[]{
      {240,10,0,0,0,0},{300,20,12,4,0,0},{420,25,20,8,6,0},{520,30,25,12,8,5}};
    const int index=wave-1,effective=c.quickSixWaves?std::min(3,index*4/6):index;
    s.seconds=c.quickSixWaves?seconds6[index]:seconds4[index];
    s.healthScale=c.quickSixWaves?health6[index]:health4[index];
    s.damageScale=c.quickSixWaves?1.f+.5f*index/5.f:damage4[index];
    s.enemyCap=c.quickSixWaves?caps6[index]:caps4[index];
    s.eliteCount=elites[effective];s.roleCounts=counts[effective];return s;
  }
  const float stage=static_cast<float>(wave-1);
  s.seconds=Safe(mode==GameMode::Traditional?c.traditionalWaveSeconds:c.endlessWaveSeconds,198,1,3600);
  s.healthScale=1+stage*.32f+stage*stage*.025f;s.damageScale=1+stage*.12f;
  s.enemyCap=30+static_cast<int>(std::min(130.f,stage*20));s.eliteCount=1+std::min(19,(wave-1)/2);
  s.roleCounts={120+static_cast<int>(std::min(600.f,stage*35)),
    wave>=2?15+std::min(100,wave):0, wave>=2?12+std::min(80,wave):0,
    wave>=3?10+std::min(80,wave):0, wave>=3?5+std::min(40,wave):0,
    wave>=4?3+std::min(20,wave):0};
  return s;
}
TrialEffects GetTrialEffects(unsigned mask,const CampaignConfig &c) {
  TrialEffects t;
  if (mask&~15u) return t;
  t.risk=Bits(mask); t.population=mask&1?1.3f:1; t.speed=mask&2?1.2f:1;
  t.elite=mask&4?1.5f:1; t.hazard=mask&8?1.4f:1;
  t.highRarityWeight=1+t.risk*Safe(c.riskRewardPerPoint,.15f,0,1); return t;
}
bool PurchaseUnlock(CampaignProfile &p,LegacyUnlock u,const CampaignConfig &c) {
  const unsigned i=static_cast<unsigned>(u);
  if (i>=p.unlocks.size() || p.unlocks[i] || p.daoYun<c.legacyUnlockCost || c.legacyUnlockCost==0) return false;
  if (u==LegacyUnlock::HeroSword) return false; // already available
  p.daoYun-=c.legacyUnlockCost; p.unlocks[i]=true; return true;
}
float PermanentBonus(const CampaignProfile &p,PermanentStat stat) {
  unsigned i=static_cast<unsigned>(stat); return i<p.bonusRanks.size()?std::min(10u,p.bonusRanks[i])*.01f:0;
}
bool PurchaseBonus(CampaignProfile &p,PermanentStat stat,const CampaignConfig &c) {
  const unsigned i=static_cast<unsigned>(stat);
  unsigned total=0; for (auto rank:p.bonusRanks) total+=std::min(rank,10u);
  if (i>=p.bonusRanks.size() || total>=10 || !c.permanentRankCost || p.daoYun<c.permanentRankCost) return false;
  p.daoYun-=c.permanentRankCost; ++p.bonusRanks[i]; return true;
}
void ApplyPermanentBonuses(const CampaignProfile &p,Stats &s) {
  // Defend the total budget even for edited/corrupt profiles.
  unsigned remaining=10;
  std::array<float,10> b{};
  for (unsigned i=0;i<b.size();++i) { unsigned n=std::min(remaining,p.bonusRanks[i]); b[i]=n*.01f; remaining-=n; }
  s.maxHp*=1+b[0]; s.armor=std::min(.9f,s.armor+b[1]); s.regen*=1+b[2]; s.power*=1+b[3];
  s.cooldownReduction=std::min(.9f,s.cooldownReduction+b[4]); s.area*=1+b[5];
  s.critChance=std::min(1.f,s.critChance+b[6]); s.moveSpeed*=1+b[7];
  s.pickupRadius*=1+b[8]; s.dashCooldown*=1-b[9];
}
Campaign::Campaign(const CampaignConfig &c):config_(c) {
  config_.traditionalWaveSeconds=Safe(c.traditionalWaveSeconds,198,1,3600);
  config_.endlessWaveSeconds=Safe(c.endlessWaveSeconds,180,1,3600);
  config_.riskRewardPerPoint=Safe(c.riskRewardPerPoint,.15f,0,1);
}
bool Campaign::Start(const CampaignProfile &p,GameMode mode,int hero,std::uint64_t seed,std::uint64_t runId,unsigned mask) {
  if (!ValidMode(mode) || !IsHeroUnlocked(p,hero) || !runId || (mask&~15u) ||
      (mode==GameMode::Endless && !HasFeature(p,Feature::Endless)) ||
      (mask && (mode==GameMode::Quick || !HasFeature(p,Feature::Trials) || Bits(mask)>std::min(p.unlockedRisk,4u))) ||
      runId<=p.settledThrough[static_cast<unsigned>(hero)]) return false;
  state_={}; state_.mode=mode; state_.hero=hero; state_.runId=runId; state_.rng=seed?seed:1;
  state_.riskMask=mask; state_.tier=FeatureTier(p); state_.phase=CampaignPhase::Wave; return true;
}
std::uint64_t Campaign::Random() { state_.rng^=state_.rng>>12; state_.rng^=state_.rng<<25; state_.rng^=state_.rng>>27; return state_.rng*2685821657736338717ULL; }
void Campaign::Offer() {
  state_.offerCount=0; state_.selected=-1; state_.opportunityResolved=false;
  if (state_.mode==GameMode::Quick) { state_.opportunityResolved=true; return; }
  std::vector<OpportunityKind> pool{OpportunityKind::Elite,OpportunityKind::Event,OpportunityKind::Cave};
  if (HasFeature(state_.tier,Feature::Chest)) pool.push_back(OpportunityKind::Chest);
  for (std::size_t i=pool.size()-1;i>0;--i) std::swap(pool[i],pool[Random()%(i+1)]);
  // Guarantees concern appearance, not which option the player chooses.
  auto force=[&](OpportunityKind k) { auto it=std::find(pool.begin(),pool.end(),k); if (it>=pool.begin()+3) std::iter_swap(it,pool.begin()); };
  if (state_.caveAbsent>=2) force(OpportunityKind::Cave);
  if (HasFeature(state_.tier,Feature::Chest) && state_.chestAbsent>=2) {
    auto it=std::find(pool.begin(),pool.end(),OpportunityKind::Chest);
    if (it>=pool.begin()+3) std::iter_swap(it,pool.begin()+1); // never evicts forced cave
  }
  std::copy_n(pool.begin(),3,state_.offers.begin()); state_.offerCount=3;
  bool cave=false,chest=false;
  for (auto o:state_.offers) { cave|=o==OpportunityKind::Cave; chest|=o==OpportunityKind::Chest; }
  state_.caveAbsent=cave?0:state_.caveAbsent+1; state_.chestAbsent=chest?0:std::min(2,state_.chestAbsent+1);
}
bool Campaign::CompleteWave() {
  if (state_.phase!=CampaignPhase::Wave) return false;
  state_.phase=CampaignPhase::Intermission; Offer(); return true;
}
OpportunityResolution Campaign::ChooseOpportunity(unsigned index) {
  OpportunityResolution r;
  if (state_.phase!=CampaignPhase::Intermission || state_.selected!=-1 || index>=state_.offerCount) return r;
  r.accepted=true; r.kind=state_.offers[index]; state_.selected=static_cast<int>(index); state_.opportunityResolved=true;
  switch(r.kind) {
  case OpportunityKind::Elite: r.eliteEncounter=true; state_.phase=CampaignPhase::OpportunityCombat; state_.opportunityResolved=false; break;
  case OpportunityKind::Event: r.xp=config_.eventXp; r.spiritStones=HasFeature(state_.tier,Feature::Economy)?config_.eventStones:0; break;
  case OpportunityKind::Cave: r.openCave=true; break;
  case OpportunityKind::Chest: r.openChest=true; break;
  }
  return r;
}
bool Campaign::CompleteOpportunity() {
  if (state_.phase!=CampaignPhase::OpportunityCombat) return false;
  state_.phase=CampaignPhase::Intermission; state_.opportunityResolved=true; return true;
}
bool Campaign::BeginNextWave() {
  if (state_.phase!=CampaignPhase::Intermission || !state_.opportunityResolved) return false;
  auto s=ScheduleWave(state_.mode,state_.wave,config_);
  const bool bossDue=s.bossAfter && (state_.mode!=GameMode::Endless || state_.bosses<state_.wave/5);
  if (bossDue) state_.phase=CampaignPhase::Boss;
  else { if (state_.wave==std::numeric_limits<int>::max()) return false; ++state_.wave; state_.phase=CampaignPhase::Wave; }
  state_.offerCount=0; state_.selected=-1; return true;
}
bool Campaign::CompleteBoss() {
  if (state_.phase!=CampaignPhase::Boss) return false;
  ++state_.bosses;
  state_.phase=state_.mode==GameMode::Endless?CampaignPhase::Intermission:CampaignPhase::Won;
  // Endless boss rewards and inventory resolve before explicit next-wave action; no second choice for the same wave.
  state_.opportunityResolved=true; return true;
}
bool Campaign::RecordKills(std::uint64_t count) {
  if (state_.phase!=CampaignPhase::Wave && state_.phase!=CampaignPhase::Boss && state_.phase!=CampaignPhase::OpportunityCombat) return false;
  state_.kills=Add(state_.kills,count); return true;
}
bool Campaign::Die() {
  if (state_.phase!=CampaignPhase::Wave && state_.phase!=CampaignPhase::Boss && state_.phase!=CampaignPhase::OpportunityCombat) return false;
  state_.phase=CampaignPhase::Dead; return true;
}
std::uint64_t Campaign::Reward() const {
  if (state_.phase!=CampaignPhase::Dead && state_.phase!=CampaignPhase::Won) return 0;
  const double waveReward=double(state_.wave)*(state_.mode==GameMode::Quick?1:2)*(1+Bits(state_.riskMask)*double(config_.riskRewardPerPoint));
  std::uint64_t result=static_cast<std::uint64_t>(std::floor(waveReward));
  if (config_.killsPerDaoYun) result=Add(result,state_.kills/config_.killsPerDaoYun);
  if (state_.mode==GameMode::Endless) result=Add(result,std::uint64_t(state_.bosses)*config_.endlessBossReward);
  return result;
}
bool Campaign::Settle(CampaignProfile &p) {
  if (state_.settled || (state_.phase!=CampaignPhase::Dead && state_.phase!=CampaignPhase::Won) ||
      state_.runId<=p.settledThrough[static_cast<unsigned>(state_.hero)]) return false;
  p.daoYun=Add(p.daoYun,Reward()); p.settledThrough[static_cast<unsigned>(state_.hero)]=state_.runId; state_.settled=true;
  const bool won=state_.phase==CampaignPhase::Won;
  p.anyWin|=won; p.traditionalWin|=won && state_.mode==GameMode::Traditional;
  if ((won || (state_.mode==GameMode::Endless && state_.bosses>0)) && state_.riskMask)
    p.unlockedRisk=std::max(std::min(p.unlockedRisk,4u),std::min(4u,Bits(state_.riskMask)+1));
  auto it=std::find_if(p.heroes.begin(),p.heroes.end(),[&](const HeroRecord &r){return r.hero==state_.hero;});
  if (it==p.heroes.end()) { p.heroes.push_back({}); it=p.heroes.end()-1; it->hero=state_.hero; }
  it->bestWave=std::max(it->bestWave,state_.wave); it->kills=Add(it->kills,state_.kills);
  it->runs=Add(it->runs,1); it->wins=Add(it->wins,won?1:0); it->mastery=Add(it->mastery,state_.wave);
  if(state_.mode==GameMode::Endless) {
    // Settlement's per-hero high watermark rejects replay before any ranking mutation.
    p.endlessRanking.push_back({state_.runId,state_.hero,state_.wave,state_.kills});
    std::sort(p.endlessRanking.begin(),p.endlessRanking.end(),[](const EndlessRecord& a,const EndlessRecord& b){
      if(a.wave!=b.wave)return a.wave>b.wave;
      if(a.kills!=b.kills)return a.kills>b.kills;
      if(a.runId!=b.runId)return a.runId<b.runId;
      return a.hero<b.hero;
    });
    if(p.endlessRanking.size()>20)p.endlessRanking.resize(20);
  }
  return true;
}
bool Campaign::Restore(const CampaignRunState &s) {
  if (!ValidMode(s.mode) || !s.runId || !s.rng || s.hero<0 || s.hero>2 || s.tier<1 || s.tier>3 ||
      !ScheduleWave(s.mode,s.wave,config_).valid || (s.riskMask&~15u) || s.offerCount>3 ||
      (s.offerCount!=0 && s.offerCount!=3) || s.selected < -1 || s.selected>=static_cast<int>(s.offerCount) ||
      s.phase<=CampaignPhase::Inactive || s.phase>CampaignPhase::Dead || s.bosses<0 ||
      s.caveAbsent<0 || s.caveAbsent>2 || s.chestAbsent<0 || s.chestAbsent>2 ||
      (s.mode==GameMode::Quick && (s.offerCount || s.riskMask)) ||
      (s.mode==GameMode::Endless && s.tier<3) || (s.riskMask && s.tier<3)) return false;
  if (s.bosses>(s.mode==GameMode::Endless?s.wave/5:1) ||
      (s.phase==CampaignPhase::Won && (s.mode==GameMode::Endless || !s.bosses)) ||
      (s.settled && s.phase!=CampaignPhase::Won && s.phase!=CampaignPhase::Dead)) return false;
  if(s.mode==GameMode::Endless) {
    const int priorBosses=(s.wave-1)/5;
    const bool afterBoss=s.phase==CampaignPhase::Intermission && s.wave%5==0 && s.bosses==s.wave/5;
    if(!afterBoss && s.bosses!=priorBosses)return false;
  } else if(s.bosses!=(s.phase==CampaignPhase::Won?1:0)) return false;
  if (s.phase==CampaignPhase::Boss && (!ScheduleWave(s.mode,s.wave,config_).bossAfter ||
      s.bosses!=(s.mode==GameMode::Endless?s.wave/5-1:0))) return false;
  if (s.phase==CampaignPhase::Won && !ScheduleWave(s.mode,s.wave,config_).bossAfter) return false;
  if (s.phase==CampaignPhase::Intermission) {
    const bool afterEndlessBoss=s.mode==GameMode::Endless && s.wave%5==0 && s.bosses==s.wave/5;
    if (s.mode!=GameMode::Quick && !afterEndlessBoss && s.offerCount!=3) return false;
    if ((s.mode==GameMode::Quick || afterEndlessBoss) && !s.opportunityResolved) return false;
    if (s.offerCount && s.opportunityResolved && s.selected<0) return false;
  }
  for (unsigned i=0;i<s.offerCount;++i) {
    if (s.offers[i]<OpportunityKind::Elite || s.offers[i]>OpportunityKind::Chest ||
        (s.offers[i]==OpportunityKind::Chest && s.tier<3)) return false;
    for (unsigned j=0;j<i;++j) if (s.offers[i]==s.offers[j]) return false;
  }
  if (s.phase==CampaignPhase::OpportunityCombat && (s.selected<0 || s.offers[s.selected]!=OpportunityKind::Elite || s.opportunityResolved)) return false;
  state_=s; return true;
}
} // namespace dywx::mvp
