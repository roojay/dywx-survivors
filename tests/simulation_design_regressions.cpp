#include "MVP/Simulation.h"
#include <iostream>
#include <algorithm>
using namespace dywx::mvp;
static int failures=0;
static void Check(bool ok,const char* what){if(!ok){++failures;std::cerr<<"FAIL "<<what<<'\n';}}
static Snapshot& Fixture(Simulation&s){return const_cast<Snapshot&>(s.GetSnapshot());}
static RunConfig Quiet(){RunConfig c;c.spawning=false;c.automaticSpells=false;return c;}
static void Advance(Simulation&s,int frames){for(int n=0;n<frames;++n)s.Step(1.f/60);}
static bool Beast(const Simulation&s){for(const auto&e:s.GetSnapshot().enemies)if(e.role==EnemyRole::SpiritBeast)return true;return false;}
int main(){
  // Actual visible combat frames, not encounter flags set by a spawn immediately cleared at wave end.
  for(auto mode:{GameMode::Quick,GameMode::Traditional}) {
    Simulation s;auto c=Quiet();c.spawning=true;c.waveDurations.fill(10);CampaignConfig cc;cc.traditionalWaveSeconds=10;
    CampaignProfile p;Check(s.StartCampaign(p,mode,HeroId::Body,1,1,CaveId::ThunderMarsh,0,c,cc),"start elite fixture");
    if(mode==GameMode::Quick){Fixture(s).wave=3;} // Isolate the scheduled Quick wave-three budget.
    int eliteFrames=0;for(int n=0;n<610;++n){s.TestSetPlayer({},100000);s.Step(1.f/60);for(auto&e:s.GetSnapshot().enemies)eliteFrames+=e.tier==EnemyTier::Elite;Fixture(s).enemies.clear();}
    Check(eliteFrames>0,"scheduled elite has a playable frame before wave end");
  }
  {
    Simulation s;auto c=Quiet();c.spawning=true;c.maxEnemies=2;CampaignConfig cc;cc.traditionalWaveSeconds=100;CampaignProfile p;
    s.StartCampaign(p,GameMode::Traditional,HeroId::Body,3,1,CaveId::ThunderMarsh,0,c,cc);
    Fixture(s).waveElapsed=Fixture(s).statistics.elapsed=70;s.TestSpawn(EnemyRole::Swarm,{40,0});s.TestSpawn(EnemyRole::Swarm,{40,1});s.Step(1.f/60);
    Check(s.GetSnapshot().enemies.front().tier!=EnemyTier::Elite,"capacity postpones due elite");
    Fixture(s).enemies.clear();s.Step(1.f/60);
    Check(s.GetSnapshot().enemies.size()<=2 && std::any_of(s.GetSnapshot().enemies.begin(),s.GetSnapshot().enemies.end(),[](const auto&e){return e.tier==EnemyTier::Elite;}),"due elite claims free capacity without lost count");
  }
  for(auto spell:{SpellId::FlameSword,SpellId::ThunderSword,SpellId::FusionSwords}){
    Simulation s;s.Start(1,HeroId::Sword,Quiet());auto far=s.TestSpawn(EnemyRole::Support,{40,0}),near=s.TestSpawn(EnemyRole::Swarm,{-4,0});
    for(auto&e:Fixture(s).enemies){e.speed=0;e.cooldown=100;e.hp=e.maxHp=e.id==far?10000:1000;}
    s.TestCast(spell,5);for(auto&p:s.GetSnapshot().projectiles)Check(p.target==near,"every legacy sword primary/secondary target is in range");
    Advance(s,120);Check(s.GetSnapshot().statistics.damageDealt>0,"reachable enemy is hit despite distant high-HP enemy");
  }
  // Same seeds and same single draw: cap pressure must delay a successful beast, not reroll it.
  int wins=0;for(int seed=1;seed<=100;++seed){
    auto c=Quiet();c.spawning=true;c.maxEnemies=1;CampaignConfig cc;cc.traditionalWaveSeconds=100;CampaignProfile p;
    Simulation full;full.StartCampaign(p,GameMode::Traditional,HeroId::Body,seed,1,CaveId::ThunderMarsh,0,c,cc);
    // At 61%, the sole elite is not due yet. A cap-failed event is exactly a
    // successful beast draw; ordinary budget spawning checks capacity first.
    Fixture(full).waveElapsed=Fixture(full).statistics.elapsed=61;
    full.TestSpawn(EnemyRole::Swarm,{40,0});full.Step(1.f/60);
    if(full.GetSnapshot().statistics.droppedEvents==0){
      Fixture(full).enemies.clear();full.Step(1.f/60);
      Check(!Beast(full),"failed beast draw is not rerolled when capacity frees");continue;
    }
    ++wins;
    const auto drops=full.GetSnapshot().statistics.droppedEvents;Advance(full,10);
    Check(full.GetSnapshot().statistics.droppedEvents==drops,"pending beast waits without repeated spawn attempts at full capacity");
    Simulation restarted=full;restarted.StartCampaign(p,GameMode::Traditional,HeroId::Body,seed,2,CaveId::ThunderMarsh,0,c,cc);
    Fixture(restarted).statistics.elapsed=10;restarted.Step(1.f/60);Check(!Beast(restarted),"Start clears still-pending beast");
    Simulation ended=full;Fixture(ended).waveElapsed=100;ended.Step(1.f/60);
    for(unsigned i=0;i<ended.GetCampaign().GetState().offerCount;++i)if(ended.GetCampaign().GetState().offers[i]==OpportunityKind::Cave){Check(ended.ChooseOpportunity(i),"choose cave after pending-beast wave");break;}
    Check(ended.BeginNextWave(),"begin following wave after pending-beast wave");ended.Step(1.f/60);Check(!Beast(ended),"wave cleanup clears pending beast");
    Fixture(full).enemies.clear();full.Step(1.f/60);Check(Beast(full),"successful beast roll retries after cap frees");
    Fixture(full).enemies.clear();full.Step(1.f/60);Check(!Beast(full),"successful beast spawns only once per wave");
  }Check(wins>0,"beast-cap test exercised successful draws");
  {
    Simulation s;auto c=Quiet();c.waveDurations.fill(.1f);CampaignProfile p;s.StartCampaign(p,GameMode::Quick,HeroId::Body,7,1,CaveId::ThunderMarsh,0,c);s.Step(.2f);
    s.GetProgression().AddXp(100);s.RefreshProgression();const int wave=s.GetCampaign().GetState().wave;
    Check(!s.BeginNextWave(),"public next-wave rejects pending upgrades");Check(s.GetSnapshot().phase==RunPhase::Rest && s.GetCampaign().GetState().wave==wave,"pending upgrade rejection does not advance campaign");
  }
  {
    Simulation s;auto c=Quiet();c.waveDurations.fill(.1f);CampaignProfile p;s.StartCampaign(p,GameMode::Quick,HeroId::Body,7,1,CaveId::ThunderMarsh,0,c);s.Step(.2f);
    auto& prog=s.GetProgression();for(int i=0;i<7;++i)prog.GrantRelic(prog.GenerateRelic(Rarity::Dao));s.RefreshProgression();
    auto id=prog.GetState().bag.front().id;Check(s.Refine({id},RefineReward::Experience),"prepare deferred refine XP");
    Check(!s.BeginNextWave(),"deferred XP causing upgrade holds rest");Check(s.GetSnapshot().phase==RunPhase::Rest && s.GetSnapshot().progression.pendingUpgrades>0,"deferred XP choices surfaced before combat");
    while(s.GetSnapshot().progression.pendingUpgrades>0)Check(s.ChooseUpgrade(0),"resolve deferred XP choice");
    Check(s.BeginNextWave(),"advance after all deferred XP choices");
  }
  std::cout<<"simulation design regressions: "<<failures<<" failures\n";return failures?1:0;
}
