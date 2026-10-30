#include "MVP/Economy.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace dywx::mvp;
int main() {
  Progression p;p.Reset(1,HeroId::Body);Economy e;e.Reset(2,true,false,false);
  e.CreditKill(EnemyTier::Elite);assert(e.Stones()==0);p.BeginRest();assert(e.BeginRest(p,1,false));assert(e.Shop().empty());
  assert(!e.OpenTreasure(p,false,true));assert(!e.CreditStones(10));
  e.EndRest(p);p.EndRest();
  e.Reset(2,true,true,true);p.BeginRest();assert(e.BeginRest(p,1,false));assert(e.Shop().size()==3);
  assert(!e.BeginRest(p,1,false));assert(!e.Refresh(p));assert(e.CreditStones(10000));
  const auto token=e.Shop()[0].token;const int cost=e.Shop()[0].price;
  assert(e.Buy(p,token));assert(e.Stones()==10000-cost);assert(!e.Buy(p,token));assert(!e.Buy(p,99999));
  for(auto& g:e.Shop())assert(g.relic.rarity!=Rarity::Dao);
  for(int i=0;i<12;++i)e.ReceiveRelic(p,p.GenerateRelic(Rarity::Spirit));
  auto id=p.GetState().bag[0].id;assert(p.SetLocked(id,true));
  assert(e.PreviewRefine(p,{id,id},RefineReward::Stones).ids.empty());assert(p.SetLocked(id,false));
  auto preview=e.PreviewRefine(p,{id,id},RefineReward::Stones);assert(preview.ids.size()==1&&preview.amount==15);
  int before=e.Stones();assert(e.Refine(p,{id,id},RefineReward::Stones));assert(e.Stones()==before+15);
  assert(!e.Refine(p,{id},RefineReward::Stones));assert(e.UndoRefine(p));assert(e.Stones()==before);assert(!e.UndoRefine(p));
  assert(e.Refine(p,{id},RefineReward::Experience));assert(e.PendingXp()==12);assert(e.UndoRefine(p));assert(e.PendingXp()==0);
  p.SetPityWaves(20);assert(e.TreasureProbabilities(p)[2]==1);assert(e.OpenTreasure(p,false,true));
  assert(!e.OpenTreasure(p,true));auto treasure=e.RevealTreasure(p);assert(treasure&&treasure->rarity==Rarity::Immortal);assert(!e.RevealTreasure(p));assert(!e.OpenTreasure(p,false,true));
  for(int i=0;i<5;++i){assert(e.OpenTreasure(p,true));assert(e.RevealTreasure(p));}assert(!e.OpenTreasure(p,true));assert(e.PaidTreasureCount()==5);
  e.EndRest(p);p.EndRest();assert(!e.Buy(p,token));assert(!e.Refine(p,{id},RefineReward::Stones));
  e.Reset(3,false,true,true);p.Reset(4,HeroId::Body);auto immortal=p.GenerateRelic(Rarity::Immortal);id=immortal.id;
  e.ReceiveRelic(p,immortal);assert(e.IsUnidentified(id));assert(p.GetState().equipped[0].affixes.empty());
  p.BeginRest();assert(e.BeginRest(p,1,false));assert(e.Shop().empty());assert(!e.Identify(p,id));assert(!e.BeginReforge(p,id,0));e.EndRest(p);p.EndRest();
  p.BeginRest();assert(e.BeginRest(p,2,true,true));assert(e.Shop().size()==4);assert(e.Refresh(p));assert(!e.Refresh(p));assert(e.Identify(p,id));assert(!e.IsUnidentified(id));assert(!e.Identify(p,id));
  assert(e.CreditStones(10000));int initial=e.ReforgePrice(id);assert(e.BeginReforge(p,id,0));assert(!e.BeginReforge(p,id,0));assert(e.ReforgePrice(id)>initial);assert(e.ResolveReforge(p,false));assert(!e.ResolveReforge(p,false));
  assert(e.BeginReforge(p,id,0));assert(e.ResolveReforge(p,true));
  for(std::size_t i=0;i<p.GetState().equipped[0].affixes.size();++i)if(p.GetState().equipped[0].affixes[i].kind==AffixKind::Mystery)assert(!e.BeginReforge(p,id,i));
  // Repeatable independent seeded runs, adversarial stale tokens, all rarities reachable.
  for(unsigned seed=0;seed<200;++seed){
    Progression a,b;a.Reset(seed,HeroId::Body);b.Reset(seed,HeroId::Body);Economy x,y;x.Reset(seed,false,true,true);y.Reset(seed,false,true,true);a.BeginRest();b.BeginRest();x.BeginRest(a,1,true);y.BeginRest(b,1,true);
    for(std::size_t i=0;i<x.Shop().size();++i){assert(x.Shop()[i].relic.catalogId==y.Shop()[i].relic.catalogId);assert(x.Shop()[i].price==y.Shop()[i].price);}
    auto stale=x.Shop()[0].token;x.CreditStones(100);assert(x.Refresh(a));assert(!x.Buy(a,stale));
    const auto odds=x.TreasureProbabilities(a);assert(std::abs(odds[0]+odds[1]+odds[2]+odds[3]-1)<1e-10);for(auto q:odds)assert(q>0);
  }
  EconomyConfig riskConfig; riskConfig.rarityWeight=1.6f;
  Economy risky; risky.Reset(1,false,true,true,riskConfig);
  p.SetPityWaves(0); auto low=e.TreasureProbabilities(p),high=risky.TreasureProbabilities(p);
  assert(high[2]>low[2]&&high[3]>low[3]);
  auto collision=e.ExportState(); collision.treasure=collision.shop[0].relic;
  Economy guard; assert(!guard.RestoreState(collision,p));
  assert(e.BeginReforge(p,id,0));auto staleAffix=e.ExportState();staleAffix.reforge->oldAffix.value+=1;
  assert(!guard.RestoreState(staleAffix,p));assert(e.ResolveReforge(p,false));
  // Spirit relics may have at most one Trigger, including after reforge/restore.
  for(unsigned seed=1;seed<=100;++seed){
    Progression rp;rp.Reset(seed,HeroId::Body);Economy re;re.Reset(seed,false,true,true);
    auto rr=rp.GenerateRelicFromCatalog(4);auto rid=rr.id;re.ReceiveRelic(rp,rr,true);
    rp.BeginRest();assert(re.BeginRest(rp,1,true));assert(re.CreditStones(100000));
    for(int attempt=0;attempt<10;++attempt)if(re.BeginReforge(rp,rid,0)){
      auto invalid=re.ExportState();invalid.reforge->newAffix={AffixKind::Trigger,0,5};
      Economy rejected;assert(!rejected.RestoreState(invalid,rp));
      assert(re.ResolveReforge(rp,true));unsigned triggers=0;
      for(const auto& affix:rp.GetState().equipped[0].affixes)triggers+=affix.kind==AffixKind::Trigger;
      assert(triggers<=1);
    }
  }
  auto saved=e.ExportState();Economy resumed;assert(resumed.RestoreState(saved,p));
  assert(resumed.Stones()==e.Stones());assert(resumed.ReforgePrice(id)==e.ReforgePrice(id));
  auto bad=saved;bad.rng="not a generator";int balance=resumed.Stones();assert(!resumed.RestoreState(bad,p));assert(resumed.Stones()==balance);
  bad=saved;bad.stones=-1;assert(!resumed.RestoreState(bad,p));
  auto pcopy=p;assert(e.Refresh(p));assert(resumed.Refresh(pcopy));
  for(std::size_t i=0;i<e.Shop().size();++i){assert(e.Shop()[i].token==resumed.Shop()[i].token);assert(e.Shop()[i].relic.catalogId==resumed.Shop()[i].relic.catalogId);}
  Progression prep;prep.Reset(17,HeroId::Body);Economy prepEconomy;prepEconomy.Reset(18,false,true,true);
  for(int i=0;i<9;++i)prepEconomy.ReceiveRelic(prep,prep.GenerateRelic(Rarity::Spirit));
  prep.BeginRest();assert(prepEconomy.BeginRest(prep,1,true));
  auto prepId=prep.GetState().bag[0].id;assert(prepEconomy.Refine(prep,{prepId},RefineReward::Experience));
  auto legacyId=prep.GetState().bag[0].id;assert(prep.Refine({legacyId}));
  assert(prepEconomy.CommitRestExperience(prep));assert(prepEconomy.PendingXp()==0&&prep.PendingRefineXp()==0);
  auto afterCommit=prep.GetState();assert(prepEconomy.CommitRestExperience(prep));
  assert(afterCommit.xp==prep.GetState().xp&&afterCommit.pendingUpgrades==prep.GetState().pendingUpgrades);
  assert(prep.IsResting()&&!prepEconomy.Shop().empty());assert(!prepEconomy.UndoRefine(prep));assert(!prep.UndoRefine());
  Economy firstLayer;firstLayer.Reset(5,false,false,false);Progression novice;novice.Reset(6,HeroId::Body);auto unknown=novice.GenerateRelic(Rarity::Immortal);
  firstLayer.ReceiveRelic(novice,unknown);novice.BeginRest();assert(firstLayer.BeginRest(novice,1,false));assert(firstLayer.EnterCave(novice));assert(firstLayer.Identify(novice,unknown.id));assert(firstLayer.Shop().empty());
  assert(!firstLayer.EnterCave(novice));assert(!firstLayer.BeginReforge(novice,unknown.id,0));
  p.GenerateBossRewards();assert(e.ChooseBossReward(p,0));assert(!e.ChooseBossReward(p,0));e.EndRun();assert(e.Stones()==0);assert(!e.OpenTreasure(p,true));
  std::cout<<"Economy tests passed\n";
}
