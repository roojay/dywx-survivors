#include "MVP/Simulation.h"
#include "MVP/SaveCodec.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
using namespace dywx::mvp;

namespace {
CampaignProfile Veteran() {
  CampaignProfile p;p.anyWin=p.traditionalWin=true;p.unlocks.fill(true);return p;
}
void Choices(Simulation& s) {
  for(int guard=0;s.GetSnapshot().progression.pendingUpgrades;++guard){
    assert(guard<100);assert(s.ChooseUpgrade(0));
  }
}
void Arrange(Simulation& s) {
  assert(s.GetSnapshot().phase==RunPhase::Rest);
  const auto c=s.GetCampaign().GetState();
  if(!c.opportunityResolved) {
    unsigned i=0;
    while(i<c.offerCount && (c.offers[i]==OpportunityKind::Elite || c.offers[i]==OpportunityKind::Chest))++i;
    assert(i<c.offerCount);assert(s.ChooseOpportunity(i));
  }
  Choices(s);
}
InputState Input(int frame) {
  // Uneven wall-clock frames exercise the fixed-step accumulator. The checkpoint
  // is still at the supported rest boundary, not a claim of mid-combat rollback.
  const float angle=float(frame)*.017f;
  return {{std::cos(angle),std::sin(angle)},frame%73==0};
}
float Delta(int frame) { return frame%3==0?.017f:frame%3==1?.039f:.061f; }
void Wave(Simulation& s) {
  for(int i=0;i<3000 && s.GetSnapshot().phase==RunPhase::Wave;++i)s.Step(Delta(i),Input(i));
  assert(s.GetSnapshot().phase==RunPhase::Rest);
}
std::string Boundary(const Simulation& s) {
  RunCheckpoint c;assert(s.PrepareCheckpoint(c));std::string b;assert(SaveCodec::EncodeCheckpoint(c,b));return b;
}
Simulation Resume(const std::string& bytes,const CampaignProfile& p) {
  RunCheckpoint c;assert(SaveCodec::DecodeCheckpoint(bytes,c));Simulation s;assert(s.RestoreCheckpoint(c,p));return s;
}
void PairWave(Simulation& a,Simulation& b) {
  for(int i=0;i<3000 && a.GetSnapshot().phase==RunPhase::Wave;++i){
    a.Step(Delta(i),Input(i));b.Step(Delta(i),Input(i));
    const auto& x=a.GetSnapshot();const auto& y=b.GetSnapshot();
    assert(x.phase==y.phase && x.waveElapsed==y.waveElapsed);
    assert(x.player.hp==y.player.hp && x.player.shield==y.player.shield);
    assert(x.player.position.x==y.player.position.x && x.player.position.y==y.player.position.y);
    assert(x.statistics.damageDealt==y.statistics.damageDealt && x.statistics.kills==y.statistics.kills);
    assert(x.enemies.size()==y.enemies.size() && x.projectiles.size()==y.projectiles.size());
    for(std::size_t j=0;j<x.enemies.size();++j){const auto& e=x.enemies[j];const auto& f=y.enemies[j];assert(e.id==f.id && e.hp==f.hp && e.position.x==f.position.x && e.position.y==f.position.y && e.mutations==f.mutations);}
  }
  assert(a.GetSnapshot().phase==RunPhase::Rest && b.GetSnapshot().phase==RunPhase::Rest);
  Arrange(a);Arrange(b);
  // Generation/tick counters and transient visual history intentionally restart
  // on resume and are not gameplay checkpoint state. BeginNextWave and Restore
  // both reset the partial-frame accumulator at this legal boundary.
  // Includes progression RNG, economy RNG, simulation RNG, arena/anomaly schedule,
  // player state, stats, cooldowns, cast counters, and next entity/relic IDs.
  assert(Boundary(a)==Boundary(b));
}
void NaturalSpawnReplay() {
  const auto p=Veteran();
  for(int mode=1;mode<=2;++mode)for(int seed:{1,7,29})for(int hero=0;hero<3;++hero)for(int cave=0;cave<3;++cave) {
    CampaignConfig cc;cc.traditionalWaveSeconds=cc.endlessWaveSeconds=3;
    Simulation direct;assert(direct.StartCampaign(p,GameMode(mode),HeroId(hero),seed,100,CaveId(cave),0,{},cc));
    Wave(direct);Arrange(direct);
    const auto bytes=Boundary(direct);assert(Boundary(direct)==bytes); // Projection does not consume RNG.
    auto resumed=Resume(bytes,p);assert(direct.BeginNextWave());PairWave(direct,resumed);
  }
}
void AnomalyAndDeferredExperience() {
  const auto p=Veteran();
  for(int cave=0;cave<3;++cave) {
    RunConfig rc;rc.spawning=false;rc.automaticSpells=false;
    CampaignConfig cc;cc.traditionalWaveSeconds=27;
    Simulation direct;assert(direct.StartCampaign(p,GameMode::Traditional,HeroId::Sword,73,101,CaveId(cave),0,rc,cc));
    Wave(direct);Arrange(direct);
    assert(direct.GetAnomalies().State().active);
    // A full bag fixture creates deferred refinement XP, exercising the save
    // projection that legitimately carries pending choices into the next wave.
    for(int i=0;i<7;++i)direct.GetEconomy().ReceiveRelic(direct.GetProgression(),direct.GetProgression().GenerateRelic(Rarity::Dao));
    direct.RefreshProgression();assert(!direct.GetSnapshot().progression.bag.empty());
    assert(direct.Refine({direct.GetSnapshot().progression.bag.back().id},RefineReward::Experience));
    const auto bytes=Boundary(direct);assert(Boundary(direct)==bytes);
    auto resumed=Resume(bytes,p);assert(resumed.GetSnapshot().progression.pendingUpgrades>0);
    assert(!direct.BeginNextWave()); // Finalizes XP, but waits for the same choices.
    Choices(direct);Choices(resumed);assert(direct.BeginNextWave());PairWave(direct,resumed);
  }
}
void KillBoss(Simulation& s) {
  assert(s.GetSnapshot().phase==RunPhase::Boss);
  EntityId id=0;for(const auto& e:s.GetSnapshot().enemies)if(e.role==EnemyRole::Boss)id=e.id;
  assert(id);s.TestDamageEnemy(id,1e9f);assert(s.GetSnapshot().bossPhase==2);s.TestDamageEnemy(id,1e9f);
  assert(s.ChooseBossReward(0));Choices(s);
}
void EndlessBossReplay() {
  const auto p=Veteran();
  for(int cave=0;cave<3;++cave) {
    RunConfig rc;rc.spawning=false;
    CampaignConfig cc;cc.endlessWaveSeconds=.1f;
    Simulation direct;assert(direct.StartCampaign(p,GameMode::Endless,HeroId::Body,29,102,CaveId(cave),0,rc,cc));
    for(int wave=1;wave<=5;++wave){Wave(direct);Arrange(direct);if(wave<5)assert(direct.BeginNextWave());}
    auto resumed=Resume(Boundary(direct),p);assert(direct.BeginNextWave());
    for(int i=0;i<20;++i){direct.Step(Delta(i),Input(i));resumed.Step(Delta(i),Input(i));}
    KillBoss(direct);KillBoss(resumed);assert(Boundary(direct)==Boundary(resumed));
    // Save again after the endless boss. This was absent from the old one-spawn
    // replay check; boss timers and old wave counters must not leak into wave 6.
    auto postBoss=Resume(Boundary(direct),p);assert(direct.BeginNextWave());PairWave(direct,postBoss);
  }
}
}
int main(){NaturalSpawnReplay();AnomalyAndDeferredExperience();EndlessBossReplay();std::cout<<"PASS checkpoint replay: 54 seeded spawn cases, 3 active-anomaly/deferred-XP cases, 3 endless boss boundaries; byte-identical next checkpoints\n";}
