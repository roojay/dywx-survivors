#include "MVP/Arena.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
int main(){
 int fallbacks=0;
 for(std::uint64_t seed=0;seed<1000;++seed){
  auto a=GenerateArena(seed,static_cast<CaveId>(seed%3));auto b=GenerateArena(seed,static_cast<CaveId>(seed%3));
  assert(ValidateArena(a));assert(a.ObstacleFraction()<=.25f);assert(a.obstacles.size()==b.obstacles.size());
  for(std::size_t i=0;i<a.obstacles.size();++i){assert(a.obstacles[i].position.x==b.obstacles[i].position.x);assert(a.obstacles[i].position.y==b.obstacles[i].position.y);assert(a.Walkable(a.Constrain(a.obstacles[i].position)));}
  assert(a.Walkable(a.Constrain({999,999})));assert(!a.Contains({48,27}));assert(a.Walkable({},10));fallbacks+=a.usedFallback;
 }
 assert(fallbacks==0);
 ArenaConfig bad;bad.gridSize=0;bad.halfWidth=std::numeric_limits<float>::quiet_NaN();bad.placementAttempts=0;
 auto l=GenerateArena(5,CaveId::ThunderMarsh,bad);assert(ValidateArena(l));assert(l.obstacles.empty());
 auto a=GenerateArena(10);auto b=GenerateArena(11);assert(a.obstacles[0].position.x!=b.obstacles[0].position.x);
 for(int i=0;i<30;++i){ArenaConfig custom;custom.halfWidth=24+i;custom.halfHeight=18+i*.5f;custom.gridSize=.5f+(i%4)*.5f;custom.cornerCut=i%9;custom.centralClearRadius=3+i%10;custom.obstacleCount=96;custom.placementAttempts=512;auto varied=GenerateArena(i,CaveId::MistForest,custom);assert(ValidateArena(varied));}
 AnomalySystem locked;locked.Reset(10,a,false);assert(locked.Step(1000,{},true).empty());assert(!locked.State().active);
 AnomalySystem s,t;s.Reset(10,a,true);t.Reset(10,a,true);std::array<bool,6> saw{};int warning=0,strikes=0,absorbed=0;
 for(int i=0;i<3600;++i){Vec2 p=s.State().anchor;auto events=s.Step(.1f,p,true);auto other=t.Step(.1f,p,true);assert(events.size()==other.size());
  if(s.State().active){auto id=s.State().id;saw[static_cast<int>(id)]=true;auto m=s.Modifiers(s.State().anchor);assert(m.playerPower>=1&&m.thunderPower>=1);
   if(id==AnomalyId::SpiritTide)assert(m.spawn>1&&m.currency>1);
   if(id==AnomalyId::Fissure)assert(m.chargers>1);
   if(id==AnomalyId::Fog)assert(m.ranged>1&&m.beasts>1&&m.visionRange<1000);
   if(id==AnomalyId::Eclipse)assert(m.nightEnemyPower>1&&m.xp>1);
   if(id==AnomalyId::Array){assert(m.playerPower>1);assert(s.Modifiers({}).playerPower==1);}
  }
  for(auto e:events){if(e.warning){warning++;assert(e.damage==0);}else{strikes++;assert(warning>=strikes);assert(e.damage>0);if(e.absorbed){absorbed++;assert(s.Modifiers(p).thunderPower>1);}}}
 }
 for(bool seen:saw)assert(seen);
 assert(warning>0&&strikes>0&&absorbed>0);
 // Whole-interval and subdivided calls produce the same deterministic state/events at a fixed position.
 AnomalySystem coarse,fine;coarse.Reset(99,a,true);fine.Reset(99,a,true);auto e=coarse.Step(300,{},false);std::size_t n=0;for(int i=0;i<300;++i)n+=fine.Step(1,{},false).size();assert(n==e.size());assert(coarse.State().id==fine.State().id);assert(coarse.State().active==fine.State().active);
 std::cout<<"Arena: 1000 seeds, connectivity, clearance, reproducibility, bounded config, six anomalies and layer gates passed\n";
}
