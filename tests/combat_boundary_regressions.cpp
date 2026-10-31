#include "MVP/Simulation.h"
#include <cmath>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
static int failures=0;
static void Check(bool ok,const char* name){if(!ok){std::cerr<<"FAIL: "<<name<<'\n';++failures;}}
static Snapshot& Fixture(Simulation& s){return const_cast<Snapshot&>(s.GetSnapshot());}
static RunConfig Quiet(){RunConfig c;c.spawning=false;c.automaticSpells=false;return c;}
int main(){
  for(auto cave:{CaveId::ThunderMarsh,CaveId::EmberMountain,CaveId::MistForest})for(auto cap:{1u,4u,80u}){
    Simulation s;auto c=Quiet();c.maxEnemies=cap;c.waveDurations.fill(.1f);
    CampaignProfile p;p.unlocks[unsigned(LegacyUnlock::RealmSecond)]=true;p.unlocks[unsigned(LegacyUnlock::RealmThird)]=true;
    Check(s.StartCampaign(p,GameMode::Quick,HeroId::Body,1,1,cave,0,c),"campaign starts");
    for(int wave=0;wave<4;++wave){s.Step(.2f);Check(s.BeginNextWave(),"next wave");}
    EntityId boss=0;for(auto&e:s.GetSnapshot().enemies)if(e.role==EnemyRole::Boss)boss=e.id;
    while(s.GetSnapshot().enemies.size()<cap)s.TestSpawn(EnemyRole::Swarm,{40,20});
    s.TestDamageEnemy(boss,100000);
    Check(s.GetSnapshot().enemies.size()<=cap,"phase two preserves enemy cap, including boss");
    s.TestDamageEnemy(boss,100000);Check(s.GetSnapshot().phase==RunPhase::Won,"phase two remains killable");
  }
  {
    Simulation s;s.Start(1,HeroId::Body,Quiet());auto id=s.TestSpawn(EnemyRole::Support,{10,10});
    ProjectileState p;p.position={6,0};p.velocity={14,0};p.speed=14;p.life=.001f;p.returning=true;p.homing=true;p.target=id;p.damage=1;
    Fixture(s).projectiles.push_back(p);s.Step(1.f/60);
    const auto start=s.GetSnapshot().projectiles.front().position;
    for(int i=0;i<10;++i)s.Step(1.f/60);
    Check(!s.GetSnapshot().projectiles.empty() && s.GetSnapshot().projectiles.front().position.x<start.x && std::abs(s.GetSnapshot().projectiles.front().position.y*start.x-s.GetSnapshot().projectiles.front().position.x*start.y)<.001f,"returning sword must not rehome to outbound enemy");
  }
  {
    Simulation s;s.Start(1,HeroId::Body,Quiet());s.TestSpawn(EnemyRole::Support,{2,0});
    ProjectileState p;p.position={1.5f,0};p.velocity={14,0};p.speed=14;p.life=1;p.returning=true;p.pierce=1;p.damage=1;
    Fixture(s).projectiles.push_back(p);s.Step(1.f/60);
    Check(!s.GetSnapshot().projectiles.empty() && s.GetSnapshot().projectiles.front().returned,"exhausted pierce starts return immediately");
  }
  {
    Simulation s;s.Start(1,HeroId::Body,Quiet());
    ProjectileState p;p.position={3,0};p.velocity={60,0};p.speed=60;p.life=.001f;p.element=Element::Water;p.radius=.05f;
    EffectState fire;fire.position={4,0};fire.radius=.05f;fire.life=4;fire.hostile=true;fire.kind=EffectKind::Burn;
    Fixture(s).projectiles.push_back(p);Fixture(s).effects.push_back(fire);s.Step(1.f/60);
    Check(s.GetSnapshot().effects.empty(),"water dispels fire crossed on final projectile segment");
  }
  {Simulation s;auto c=Quiet();c.fixedStep=std::numeric_limits<float>::quiet_NaN();s.Start(1,HeroId::Body,c);s.Step(.1f);Check(s.GetSnapshot().tick>0,"invalid fixed step does not freeze simulation");}
  std::cout<<"combat boundary regressions: "<<failures<<" failures\n";return failures?1:0;
}
