#include "MVP/Simulation.h"
#include "MVP/SaveCodec.h"
#include <cassert>
#include <iostream>
#include <sstream>
using namespace dywx::mvp;
static unsigned EliteIndex(const Simulation& s) {
  const auto& c=s.GetCampaign().GetState();
  for(unsigned i=0;i<c.offerCount;++i)if(c.offers[i]==OpportunityKind::Elite)return i;
  assert(false);return 0;
}
static void DrainUpgrades(Simulation& s) {
  for(int limit=0;s.GetSnapshot().progression.pendingUpgrades>0;++limit){
    assert(limit<100);assert(s.ChooseUpgrade(0));
  }
}
static Simulation Ready() {
  CampaignProfile p;p.anyWin=p.traditionalWin=true;
  RunConfig r;r.spawning=false;r.automaticSpells=false;
  CampaignConfig c;c.traditionalWaveSeconds=.1f;
  Simulation s;assert(s.StartCampaign(p,GameMode::Traditional,HeroId::Body,1,1,CaveId::ThunderMarsh,0,r,c));
  for(int i=0;i<20&&s.GetSnapshot().phase==RunPhase::Wave;++i)s.Step(.1f);
  assert(s.GetSnapshot().phase==RunPhase::Rest);return s;
}
static std::string Boundary(const Simulation& s) {
  std::ostringstream out;const auto& x=s.GetSnapshot();const auto& c=s.GetCampaign().GetState();
  out<<int(x.phase)<<':'<<x.wave<<':'<<x.waveElapsed<<':'<<x.statistics.elapsed<<':'<<x.enemies.size()
     <<':'<<int(c.phase)<<':'<<c.selected<<':'<<c.opportunityResolved<<':'<<s.GetEconomy().Stones()
     <<':'<<s.GetEconomy().PendingXp()<<':'<<x.progression.pendingUpgrades<<':'<<s.GetProgression().IsResting();
  return out.str();
}
int main() {
  auto s=Ready();const auto initial=Boundary(s);
  assert(!s.ChooseOpportunity(99));assert(Boundary(s)==initial);
  s.Pause(true);assert(!s.ChooseOpportunity(EliteIndex(s)));assert(Boundary(s)==initial);s.Pause(false);
  assert(s.ChooseOpportunity(EliteIndex(s)));
  assert(s.GetSnapshot().phase==RunPhase::Rest&&s.GetProgression().IsResting());
  assert(s.GetSnapshot().enemies.empty());
  assert(s.GetCampaign().GetState().phase==CampaignPhase::OpportunityCombat);
  const auto selected=Boundary(s);assert(!s.ChooseOpportunity(EliteIndex(s)));assert(Boundary(s)==selected);
  RunCheckpoint checkpoint;assert(!s.PrepareCheckpoint(checkpoint));assert(Boundary(s)==selected);
  // The selected opportunity remains frozen and exposes the standard inventory commands.
  for(int i=0;i<8;++i)s.GetEconomy().ReceiveRelic(s.GetProgression(),s.GetProgression().GenerateRelic(Rarity::Common));
  s.RefreshProgression();assert(s.GetSnapshot().progression.bag.size()==2);
  assert(s.SortBag());const auto bag=s.GetSnapshot().progression.bag;
  assert(s.Equip(bag[0].id,0));assert(s.Refine({bag[1].id},RefineReward::Stones));assert(s.UndoRefine());
  auto frozen=Boundary(s);s.Step(.2f);assert(Boundary(s)==frozen);
  s.Pause(true);assert(!s.BeginNextWave());assert(Boundary(s)==frozen);s.Pause(false);
  // Fixtures may prepare a transaction directly; the public encounter command must
  // still reject it, even though the normal Elite UI never grants cave services.
  assert(s.GetEconomy().EnterCave(s.GetProgression(),false));
  assert(s.GetEconomy().CreditStones(1000));
  auto id=s.GetSnapshot().progression.equipped.front().id;
  assert(s.BeginReforge(id,0));auto transaction=Boundary(s);
  assert(!s.BeginNextWave());assert(Boundary(s)==transaction);assert(s.GetEconomy().PendingReforge());
  assert(s.ResolveReforge(false));assert(s.OpenTreasure());transaction=Boundary(s);
  assert(!s.BeginNextWave());assert(Boundary(s)==transaction);assert(s.RevealTreasure());
  s.GetProgression().AddXp(100);s.RefreshProgression();assert(s.GetSnapshot().progression.pendingUpgrades>0);
  auto pending=Boundary(s);assert(!s.BeginNextWave());assert(Boundary(s)==pending);DrainUpgrades(s);
  const int wave=s.GetSnapshot().wave;
  assert(s.BeginNextWave());assert(s.GetSnapshot().phase==RunPhase::Wave);
  assert(s.GetSnapshot().wave==wave&&!s.GetProgression().IsResting());
  assert(!s.GetSnapshot().enemies.empty());assert(!s.BeginNextWave());assert(!s.PrepareCheckpoint(checkpoint));
  for(int limit=0;!s.GetSnapshot().enemies.empty();++limit){
    assert(limit<50);s.TestDamageEnemy(s.GetSnapshot().enemies.front().id,1e9f);
  }
  assert(s.GetSnapshot().phase==RunPhase::Rest);assert(s.GetSnapshot().wave==wave);
  assert(s.GetCampaign().GetState().opportunityResolved);assert(s.SortBag());
  DrainUpgrades(s);assert(s.PrepareCheckpoint(checkpoint));assert(checkpoint.campaign.wave==wave+1);
  assert(s.BeginNextWave());assert(s.GetSnapshot().wave==wave+1);
  std::cout<<"PASS: elite selection, inventory, transaction/pause/upgrade guards, explicit battle, no wave skip, checkpoint boundary\n";
}
