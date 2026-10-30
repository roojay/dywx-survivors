#include "MVP/Campaign.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
static void Rest(Campaign &c) {
  if(c.GetState().offerCount) {
    auto r=c.ChooseOpportunity(0); assert(r.accepted);
    assert(!c.ChooseOpportunity(0).accepted);
    if(r.eliteEncounter) { assert(!c.BeginNextWave()); assert(c.CompleteOpportunity()); assert(!c.CompleteOpportunity()); }
  }
  assert(c.BeginNextWave());
}
static void Win(Campaign &c,int waves) {
  for(int i=1;i<=waves;++i) {
    assert(c.GetState().wave==i); assert(c.RecordKills(100));
    assert(c.CompleteWave()); assert(!c.CompleteWave()); Rest(c);
  }
  assert(c.GetState().phase==CampaignPhase::Boss); assert(c.CompleteBoss()); assert(!c.CompleteBoss());
}
int main() {
  CampaignProfile ids; assert(ReserveRunId(ids)==1); assert(ReserveRunId(ids)==2);
  Campaign early,later;
  assert(early.Start(ids,GameMode::Quick,0,1,1)); assert(later.Start(ids,GameMode::Quick,1,1,2));
  assert(later.Die() && later.Settle(ids)); assert(early.Die() && early.Settle(ids));
  assert(!early.Start(ids,GameMode::Quick,0,1,1));
  ids.nextRunId=std::numeric_limits<std::uint64_t>::max(); assert(ReserveRunId(ids)==0);
  CampaignProfile p; Campaign c;
  assert(FeatureTier(p)==1); assert(HasFeature(p,Feature::Inventory));
  assert(!HasFeature(p,Feature::Economy)); assert(!HasFeature(p,static_cast<Feature>(99)));
  assert(!c.Start(p,GameMode::Endless,0,1,1));
  assert(!c.Start(p,GameMode::Quick,0,1,0));
  assert(!c.Start(p,GameMode::Quick,0,1,1,1));
  assert(!c.Start(p,static_cast<GameMode>(-1),0,1,1));
  assert(c.Start(p,GameMode::Quick,0,1,1));
  assert(!c.ChooseOpportunity(0).accepted); assert(!c.BeginNextWave()); assert(!c.Settle(p));
  Win(c,4); assert(c.Reward()==4); auto replay=c.GetState();
  assert(c.Settle(p)); assert(!c.Settle(p)); assert(FeatureTier(p)==2 && p.daoYun==4);
  Campaign duplicate; assert(duplicate.Restore(replay)); assert(!duplicate.Settle(p));
  assert(!c.Start(p,GameMode::Quick,0,1,1));
  assert(c.Start(p,GameMode::Traditional,1,10,2)); Win(c,10);
  assert(c.Reward()==20); assert(c.Settle(p)); assert(FeatureTier(p)==3 && p.daoYun==24);
  assert(p.heroes.size()==2 && p.heroes[1].bestWave==10 && p.heroes[1].mastery==10);
  assert(!c.Start(p,GameMode::Endless,0,1,3,3));
  assert(c.Start(p,GameMode::Traditional,0,1,3,1)); Win(c,10); assert(c.Reward()==23); assert(c.Settle(p)); assert(p.unlockedRisk==2);
  assert(c.Start(p,GameMode::Endless,0,987,4,3));
  int caveGap=0,chestGap=0;
  for(int wave=1;wave<=101;++wave) {
    assert(c.GetState().wave==wave); assert(c.GetState().phase==CampaignPhase::Wave);
    assert(c.CompleteWave()); assert(!c.BeginNextWave());
    bool cave=false,chest=false;
    for(auto kind:c.GetState().offers) { cave|=kind==OpportunityKind::Cave; chest|=kind==OpportunityKind::Chest; }
    caveGap=cave?0:caveGap+1; chestGap=chest?0:chestGap+1; assert(caveGap<3 && chestGap<3);
    Campaign restored; assert(restored.Restore(c.GetState()));
    auto a=c.ChooseOpportunity(0),b=restored.ChooseOpportunity(0); assert(a.kind==b.kind && a.xp==b.xp);
    if(a.eliteEncounter) { assert(c.CompleteOpportunity()); assert(restored.CompleteOpportunity()); }
    assert(c.BeginNextWave()); assert(restored.BeginNextWave());
    if(wave%5==0) { assert(c.GetState().phase==CampaignPhase::Boss); assert(c.CompleteBoss()); assert(restored.CompleteBoss()); assert(c.BeginNextWave()); assert(restored.BeginNextWave()); }
    assert(c.GetState().rng==restored.GetState().rng);
  }
  assert(c.Die()); assert(!c.Die()); assert(c.Settle(p)); assert(p.unlockedRisk==3);
  assert(c.GetState().bosses==20 && c.GetState().wave==102);
  assert(c.Reward()==static_cast<unsigned>(std::floor(102*2*(1+2*double(c.GetConfig().riskRewardPerPoint))))+100);
  for(int wave: {1,10,100,1000000,std::numeric_limits<int>::max()}) {
    auto s=ScheduleWave(GameMode::Endless,wave); assert(s.valid && std::isfinite(s.healthScale) && s.enemyCap<=160 && s.eliteCount<=20);
  }
  auto quickOne=ScheduleWave(GameMode::Quick,1),quickFour=ScheduleWave(GameMode::Quick,4);
  assert(quickOne.roleCounts[0]==240&&quickOne.eliteCount==0&&quickOne.enemyCap==40);
  assert(quickFour.roleCounts[0]==520&&quickFour.eliteCount==2&&quickFour.enemyCap==130);
  assert(quickFour.healthScale==3.2f&&quickFour.damageScale==1.5f);
  CampaignConfig six;six.quickSixWaves=true;Campaign sixRun(six);CampaignProfile sixProfile;
  assert(sixRun.Start(sixProfile,GameMode::Quick,0,1,1));Win(sixRun,6);assert(sixRun.Reward()==6);
  assert(ScheduleWave(GameMode::Quick,6,six).seconds==95&&ScheduleWave(GameMode::Quick,6,six).bossAfter);
  assert(ScheduleWave(GameMode::Quick,6,six).healthScale==3.2f&&!ScheduleWave(GameMode::Quick,7,six).valid);
  assert(!ScheduleWave(GameMode::Traditional,11).valid); assert(!ScheduleWave(GameMode::Quick,0).valid);
  float duration=0; for(int wave=1;wave<=10;++wave) duration+=ScheduleWave(GameMode::Traditional,wave).seconds;
  assert(duration>=1800 && duration<=2280);
  p.daoYun=1000; assert(PurchaseUnlock(p,LegacyUnlock::HeroBeast)); assert(IsHeroUnlocked(p,2));
  assert(!PurchaseUnlock(p,LegacyUnlock::HeroBeast)); assert(!PurchaseUnlock(p,static_cast<LegacyUnlock>(999)));
  assert(PurchaseUnlock(p,LegacyUnlock::RealmSecond)); assert(IsRealmUnlocked(p,1));
  for(int i=0;i<10;++i) assert(PurchaseBonus(p,PermanentStat::Power));
  assert(!PurchaseBonus(p,PermanentStat::Health)); assert(!PurchaseBonus(p,static_cast<PermanentStat>(99)));
  Stats stats; stats.luck=.2f; ApplyPermanentBonuses(p,stats); assert(stats.power<=1.10001f && stats.luck==.2f);
  auto bad=replay; bad.rng=0; assert(!duplicate.Restore(bad));
  bad=replay; bad.wave=-1; assert(!duplicate.Restore(bad));
  bad=replay; bad.selected=99; assert(!duplicate.Restore(bad));
  bad=replay; bad.riskMask=16; assert(!duplicate.Restore(bad));
  Campaign death; assert(death.Start(p,GameMode::Quick,0,3,999)); assert(death.Die()); assert(death.Reward()==1); assert(death.Settle(p));
  // Local endless ranking: no finite-mode pollution, bounded, cross-hero, deterministic ties.
  assert(p.endlessRanking.size()==1&&p.endlessRanking[0].wave==102);
  CampaignProfile ranked;ranked.anyWin=ranked.traditionalWin=true;ranked.unlocks.fill(true);
  for(int i=1;i<=25;++i){
    Campaign entry;auto rid=ReserveRunId(ranked);
    assert(entry.Start(ranked,GameMode::Endless,i%3,42,rid));
    auto rs=entry.GetState();rs.wave=i<=23?i:23;rs.bosses=(rs.wave-1)/5;assert(entry.Restore(rs));
    assert(entry.RecordKills(i>=23?100:static_cast<unsigned>(i)));assert(entry.Die());
    assert(entry.Settle(ranked));assert(!entry.Settle(ranked));
    assert(ranked.endlessRanking.size()<=20);
  }
  assert(ranked.endlessRanking.size()==20);
  assert(ranked.endlessRanking[0].wave==23&&ranked.endlessRanking[0].runId==23);
  assert(ranked.endlessRanking[1].runId==24&&ranked.endlessRanking[2].runId==25);
  for(std::size_t i=1;i<ranked.endlessRanking.size();++i)assert(ranked.endlessRanking[i-1].wave>=ranked.endlessRanking[i].wave);
  assert(ranked.heroes.size()==3);
  Campaign tieKills;auto tieId=ReserveRunId(ranked);assert(tieKills.Start(ranked,GameMode::Endless,0,42,tieId));
  auto tieState=tieKills.GetState();tieState.wave=23;tieState.bosses=4;assert(tieKills.Restore(tieState));
  assert(tieKills.RecordKills(101)&&tieKills.Die()&&tieKills.Settle(ranked));
  assert(ranked.endlessRanking.size()==20&&ranked.endlessRanking[0].runId==tieId&&ranked.endlessRanking[1].runId==23);
  Campaign invalidBossHistory;CampaignRunState skipped;skipped.runId=999;skipped.mode=GameMode::Endless;skipped.wave=23;skipped.tier=3;skipped.phase=CampaignPhase::Wave;assert(!invalidBossHistory.Restore(skipped));
  Campaign finite;assert(finite.Start(ranked,GameMode::Quick,0,1,ReserveRunId(ranked)));assert(finite.Die());assert(finite.Settle(ranked));
  assert(ranked.endlessRanking.size()==20&&ranked.endlessRanking[0].runId==tieId);
  // Stress seeded guarantees and independent restore streams.
  for(unsigned seed=1;seed<=300;++seed) {
    Campaign a; assert(a.Start(p,GameMode::Endless,0,seed,10000+seed)); int cg=0,hg=0;
    for(int w=1;w<=30;++w) {
      assert(a.CompleteWave()); bool ca=false,ch=false;
      for(auto kind:a.GetState().offers) { ca|=kind==OpportunityKind::Cave; ch|=kind==OpportunityKind::Chest; }
      cg=ca?0:cg+1; hg=ch?0:hg+1; assert(cg<3 && hg<3); Rest(a);
      if(w%5==0) { assert(a.CompleteBoss()); assert(a.BeginNextWave()); }
    }
  }
  std::cout<<"campaign: modes, guarantees, rewards, gates, risk, inheritance, replay passed\n";
}
