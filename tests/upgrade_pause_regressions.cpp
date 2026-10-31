#include "MVP/Simulation.h"
#include "MVP/SaveCodec.h"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace dywx::mvp;

static RunConfig Quiet() {
  RunConfig c; c.spawning=false; c.automaticSpells=false; return c;
}
static CampaignProfile Veteran() {
  CampaignProfile p; p.anyWin=p.traditionalWin=true; p.unlocks.fill(true); return p;
}
static Snapshot& Fixture(Simulation& s) { return const_cast<Snapshot&>(s.GetSnapshot()); }
static std::string Combat(const Simulation& s) {
  const auto& v=s.GetSnapshot(); const auto& p=v.player;
  std::ostringstream o; o<<std::setprecision(9);
  o<<v.tick<<':'<<int(v.phase)<<':'<<v.wave<<':'<<v.waveElapsed<<':'<<v.statistics.elapsed
   <<':'<<p.position.x<<':'<<p.position.y<<':'<<p.hp<<':'<<p.shield<<':'<<p.invulnerable
   <<':'<<p.dashCooldown<<':'<<p.dashRemaining<<':'<<v.statistics.damageTaken<<':'<<v.statistics.damageDealt;
  for(auto x:v.spellCooldowns)o<<':'<<x;
  for(const auto& e:v.enemies)o<<"e"<<e.id<<':'<<e.position.x<<':'<<e.position.y<<':'<<e.hp<<':'<<e.cooldown<<':'<<e.windup<<':'<<e.stun<<':'<<e.burnRemaining;
  for(const auto& x:v.projectiles)o<<"p"<<x.id<<':'<<x.position.x<<':'<<x.position.y<<':'<<x.life<<':'<<x.pathTick<<':'<<x.orbitRemaining;
  for(const auto& e:v.effects)o<<"f"<<e.id<<':'<<e.position.x<<':'<<e.position.y<<':'<<e.life<<':'<<e.tick<<':'<<e.warning;
  for(const auto& x:v.pickups)o<<"x"<<x.id<<':'<<x.position.x<<':'<<x.position.y<<':'<<x.xp;
  const auto& a=s.GetAnomalies().State();
  o<<"a"<<a.active<<':'<<int(a.id)<<':'<<a.elapsed<<':'<<a.remaining<<':'<<a.warningRemaining;
  return o.str();
}
static void Frozen(Simulation& s) {
  const auto before=Combat(s);
  for(int i=0;i<80;++i)s.Step(.25f,{{1,1},true});
  assert(Combat(s)==before);
}
static void Resolve(Simulation& s) {
  for(int i=0;s.GetSnapshot().progression.pendingUpgrades;++i) {
    assert(i<100); assert(s.ChooseUpgrade(0));
  }
}
static void WaitingAndCommands() {
  Simulation s; assert(s.StartCampaign(Veteran(),GameMode::Traditional,HeroId::Body,7,1,CaveId::ThunderMarsh,0,Quiet()));
  for(int i=0;i<140;++i)s.Step(.2f); // A real active anomaly is part of the frozen state.
  assert(s.GetAnomalies().State().active);
  s.TestSpawn(EnemyRole::Swarm,{8,0});
  ProjectileState p;p.id=100;p.hostile=true;p.position={5,0};p.velocity={-4,0};p.life=10;p.damage=20;
  Fixture(s).projectiles.push_back(p);
  EffectState e;e.id=101;e.hostile=true;e.position={};e.warning=.2f;e.life=2;e.damage=20;
  Fixture(s).effects.push_back(e); Fixture(s).player.dashCooldown=3;
  s.GetProgression().AddXp(100);s.RefreshProgression();
  assert(s.GetSnapshot().progression.pendingUpgrades>=3);
  assert(!s.GetSnapshot().paused); // Manual/focus flag is independent.
  Frozen(s);
  assert(s.Reroll());Frozen(s);
  assert(s.SealUpgrade(0));Frozen(s);
  const int pending=s.GetSnapshot().progression.pendingUpgrades;
  assert(s.SkipUpgrade());assert(s.GetSnapshot().progression.pendingUpgrades==pending-1);Frozen(s);
  assert(s.ChooseUpgrade(0));assert(s.GetSnapshot().progression.pendingUpgrades>0);Frozen(s);
  // Manual pause and focus loss both enter through Pause(true) in the adapter.
  // Neither selection commands nor the automatic gate may release that flag.
  s.Pause(true);Frozen(s);
  assert(!s.ChooseUpgrade(0) && !s.Reroll() && !s.SkipUpgrade() && !s.SealUpgrade(0));
  assert(s.GetSnapshot().paused);
  s.Pause(false);Frozen(s); // Explicit continue still waits for the choice.
  Resolve(s);assert(!s.GetSnapshot().paused);
  const auto tick=s.GetSnapshot().tick;s.Step(s.GetConfig().fixedStep,{});
  assert(s.GetSnapshot().tick==tick+1); // No catch-up of the 20 seconds spent reading.
  assert(s.GetSnapshot().player.dashRemaining==0); // No queued dash leaks from the wait.
}
static void FirstSubstepAndOverflow() {
  for(bool overflow:{false,true}) {
    auto config=Quiet();config.maxPickups=1;
    Simulation s;s.Start(11,HeroId::Sword,config);
    PickupState p;p.id=200;p.position=overflow?Vec2{40,20}:Vec2{};p.xp=overflow?0:23;
    Fixture(s).pickups.push_back(p);
    if(overflow) {
      s.GetProgression().AddXp(22/s.GetSnapshot().progression.build.stats.xpMultiplier);s.RefreshProgression();
      assert(s.GetSnapshot().progression.pendingUpgrades==0);
      s.TestSpawn(EnemyRole::Swarm,{2,0});Fixture(s).enemies.back().hp=0;
    }
    s.Step(.25f);
    assert(!s.GetSnapshot().progression.offers.empty());
    assert(s.GetSnapshot().tick==1); // Stop the remaining fixed substeps immediately.
    Frozen(s);Resolve(s);
    const auto tick=s.GetSnapshot().tick;
    s.Step(config.fixedStep*.4f);assert(s.GetSnapshot().tick==tick);
    s.Step(config.fixedStep*.6f);assert(s.GetSnapshot().tick==tick+1);
  }
}
static void ExplicitPauseAndHeldDash() {
  Simulation s;s.Start(13,HeroId::Body,Quiet());s.Pause(true);
  s.GetProgression().AddXp(23);s.RefreshProgression();
  Frozen(s);assert(s.GetSnapshot().paused && !s.ChooseUpgrade(0));
  s.Pause(false);Frozen(s);Resolve(s);
  // The same held key that was observed during selection is not a new dash edge.
  s.Step(s.GetConfig().fixedStep,{{1,0},true});
  assert(s.GetSnapshot().player.dashCooldown==0);
  s.Step(s.GetConfig().fixedStep,{});
  s.Step(s.GetConfig().fixedStep,{{1,0},true});
  assert(s.GetSnapshot().player.dashCooldown>0);

  auto c=Quiet();c.waveDurations.fill(.1f);
  Simulation boss;boss.Start(17,HeroId::Body,c);
  for(int i=0;i<4;++i){boss.Step(.2f);assert(boss.BeginNextWave());}
  assert(boss.GetSnapshot().phase==RunPhase::Boss);
  boss.GetProgression().AddXp(23);boss.RefreshProgression();Frozen(boss);Resolve(boss);
  const auto tick=boss.GetSnapshot().tick;boss.Step(c.fixedStep);
  assert(boss.GetSnapshot().tick==tick+1);
}
static void CheckpointChoices() {
  auto p=Veteran();CampaignConfig cc;cc.traditionalWaveSeconds=.1f;
  Simulation s;assert(s.StartCampaign(p,GameMode::Traditional,HeroId::Sword,31,2,CaveId::MistForest,0,Quiet(),cc));
  for(int i=0;i<20 && s.GetSnapshot().phase==RunPhase::Wave;++i)s.Step(.1f);
  assert(s.GetSnapshot().phase==RunPhase::Rest);
  const auto c=s.GetCampaign().GetState();
  unsigned i=0;while(i<c.offerCount && (c.offers[i]==OpportunityKind::Elite || c.offers[i]==OpportunityKind::Chest))++i;
  assert(i<c.offerCount && s.ChooseOpportunity(i));Resolve(s);
  s.GetProgression().AddXp(100);s.RefreshProgression();
  RunCheckpoint save;assert(s.PrepareCheckpoint(save));
  std::string bytes;assert(SaveCodec::EncodeCheckpoint(save,bytes));
  RunCheckpoint decoded;assert(SaveCodec::DecodeCheckpoint(bytes,decoded));
  Simulation resumed;assert(resumed.RestoreCheckpoint(decoded,p));
  assert(resumed.GetSnapshot().phase==RunPhase::Wave && resumed.GetSnapshot().progression.pendingUpgrades>0);
  Frozen(resumed);Resolve(resumed);resumed.Step(resumed.GetConfig().fixedStep);
  assert(resumed.GetSnapshot().tick==1);
  assert(!s.BeginNextWave());Resolve(s);assert(s.BeginNextWave());
}
int main() {
  WaitingAndCommands();FirstSubstepAndOverflow();CheckpointChoices();ExplicitPauseAndHeldDash();
  std::cout<<"PASS upgrade wait: frozen combat/anomalies, command legality, queued choices, first substep, overflow, explicit pause, dash/accumulator reset and checkpoint restore\n";
}
