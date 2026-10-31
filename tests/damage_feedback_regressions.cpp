#include "MVP/Simulation.h"
#include <cmath>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
static int failures=0;
static void Check(bool ok,const char* name){if(!ok){++failures;std::cerr<<"FAIL "<<name<<'\n';}}
static Snapshot& Fixture(Simulation&s){return const_cast<Snapshot&>(s.GetSnapshot());}
static RunConfig Quiet(){RunConfig c;c.spawning=false;c.automaticSpells=false;return c;}
static Simulation Fresh(){Simulation s;s.Start(1,HeroId::Body,Quiet());return s;}
static bool Near(float a,float b){return std::abs(a-b)<.001f;}
int main(){
  {
    auto s=Fresh();auto &v=Fixture(s);v.player.shield=100;
    s.TestDamagePlayer(10);auto hit=v.lastDamage;
    Check(hit.kind==DamageKind::Unknown && hit.hpLost==0 && hit.shieldLost>0 && !hit.lethal,"shield-only actual hit recorded separately");
    v.player.invulnerable=1;s.TestDamagePlayer(50);
    Check(v.lastDamage.shieldLost==hit.shieldLost && v.player.shield==100-hit.shieldLost,"invulnerability preserves last actual hit");
    v.player.invulnerable=0;v.player.shield=0;s.TestSetPlayer({},2);s.TestDamagePlayer(1000);
    Check(v.phase==RunPhase::Dead && v.lastDamage.lethal && Near(v.lastDamage.hpLost,2),"lethal feedback reports actual HP lost, not overkill");
    s.TestDamagePlayer(1);Check(Near(v.lastDamage.hpLost,2),"later same-tick hit cannot replace lethal feedback");
    s.Restart();Check(s.GetSnapshot().lastDamage.kind==DamageKind::None,"restart clears prior run feedback");
  }
  {
    auto s=Fresh();const auto hp=s.GetSnapshot().player.hp;
    for(float d:{0.f,-5.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})s.TestDamagePlayer(d);
    Check(s.GetSnapshot().player.hp==hp && s.GetSnapshot().lastDamage.kind==DamageKind::None && s.GetSnapshot().statistics.damageTaken==0,"invalid and empty hits neither heal nor corrupt feedback");
  }
  for(auto role:{EnemyRole::Swarm,EnemyRole::Charger,EnemyRole::Exploder}){
    auto s=Fresh();const auto id=s.TestSpawn(role,{});auto &e=Fixture(s).enemies.back();e.speed=0;
    if(role==EnemyRole::Charger)e.chargeRemaining=1;
    if(role==EnemyRole::Exploder)e.windup=.001f;
    s.Step(1.f/60);const auto &hit=s.GetSnapshot().lastDamage;
    const auto expected=role==EnemyRole::Swarm?DamageKind::Contact:role==EnemyRole::Charger?DamageKind::Charge:DamageKind::Explosion;
    Check(hit.kind==expected && hit.source==id && hit.hpLost>0,"actual enemy attack tagged with attack kind and source");
  }
  {
    auto s=Fresh();ProjectileState p;p.hostile=true;p.damage=7;p.position={};p.owner=999;p.life=1;
    Fixture(s).projectiles.push_back(p);s.Step(1.f/60);
    // A projectile can hit after its shooter is removed. Subsequent ground burn
    // is a separate damage source and may hit in the same step; isolate it here.
    auto &v=Fixture(s);v.effects.clear();p.damage=1000;p.life=1;v.projectiles.push_back(p);s.Step(1.f/60);
    Check(v.lastDamage.kind==DamageKind::Projectile && v.lastDamage.source==999 && v.lastDamage.lethal,"projectile attribution survives missing shooter and same-step ground effect");
  }
  {
    auto s=Fresh();EffectState fx;fx.hostile=true;fx.kind=EffectKind::Burn;fx.position={};fx.damage=3;fx.life=2;fx.oneShot=false;fx.interval=.1f;
    Fixture(s).effects.push_back(fx);s.Step(1.f/60);const auto first=s.GetSnapshot().lastDamage;
    for(int n=0;n<8;++n)s.Step(1.f/60);
    const auto &next=s.GetSnapshot().lastDamage;
    Check(first.kind==DamageKind::HostileArea && first.effectKind==EffectKind::Burn && next.tick>first.tick && next.hpLost>0,"periodic ground damage updates effect type and event tick");
  }
  {
    auto s=Fresh();const auto id=s.TestSpawn(EnemyRole::Swarm,{-1.7f,0});auto &e=Fixture(s).enemies.back();e.mutations=8;e.speed=0;
    s.Step(1.f/60);Check(s.GetSnapshot().lastDamage.kind==DamageKind::Orbit && s.GetSnapshot().lastDamage.source==id,"orbit mutation is not mislabelled contact");
  }
  {
    Simulation s;CampaignProfile profile;profile.anyWin=true;
    CampaignConfig campaign;campaign.traditionalWaveSeconds=1000;
    Check(s.StartCampaign(profile,GameMode::Traditional,HeroId::Body,999,1,CaveId::ThunderMarsh,0,Quiet(),campaign),"start anomaly feedback fixture");
    bool lightning=false,fissure=false;
    for(int n=0;n<13000 && !(lightning&&fissure);++n){
      const auto anomaly=s.GetAnomalies().State();
      s.TestSetPlayer(anomaly.anchor,10000);s.Step(1.f/60);
      lightning |= s.GetSnapshot().lastDamage.kind==DamageKind::Lightning;
      fissure |= s.GetSnapshot().lastDamage.kind==DamageKind::Fissure;
    }
    Check(lightning&&fissure,"actual anomaly strikes distinguish lightning and fissure");
  }
  std::cout<<"damage feedback regressions: "<<failures<<" failures\n";return failures?1:0;
}
