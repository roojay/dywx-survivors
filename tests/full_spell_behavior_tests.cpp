#include "MVP/Simulation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace dywx::mvp;
static Snapshot& Fixture(Simulation& s){return const_cast<Snapshot&>(s.GetSnapshot());}
static void Setup(Simulation& s,SpellId spell,int rank=5){
  RunConfig c;c.spawning=false;c.automaticSpells=false;c.waveDurations.fill(100);s.Start(51,HeroId::Beast,c);
  auto& state=const_cast<ProgressionState&>(s.GetProgression().GetState());state.build.spells={{spell,rank}};state.build.passives.clear();
  Stats base;base.critChance=0;s.GetProgression().ConfigureFullGame(3,base);s.RefreshProgression();s.TestSetPlayer({0,0},50);
}
static EntityId Enemy(Simulation& s,Vec2 at,float speed=0){
  auto id=s.TestSpawn(EnemyRole::Swarm,at);for(auto& e:Fixture(s).enemies)if(e.id==id){e.hp=e.maxHp=10000;e.speed=speed;e.damage=0;e.cooldown=100;}
  return id;
}
static const EnemyState& Find(const Simulation&s,EntityId id){for(const auto&e:s.GetSnapshot().enemies)if(e.id==id)return e;assert(false);return s.GetSnapshot().enemies[0];}
static void Advance(Simulation&s,float seconds){for(int i=0;i<int(seconds*60+.5f);++i)s.Step(1.f/60);}
int main(){
  // A cone is facing-sensitive, actually heals, slows, and stuns on contact.
  Simulation cone;Setup(cone,SpellId::WoodPalm);auto front=Enemy(cone,{2,0}),back=Enemy(cone,{-2,0});cone.TestCast(SpellId::WoodPalm,5);
  assert(Find(cone,front).hp<10000 && Find(cone,back).hp==10000);assert(Find(cone,front).slowFraction==.25f);assert(Find(cone,front).stun>0);assert(cone.GetSnapshot().player.hp>50);
  // Field durations and rank-five follow behavior are consumed by simulation.
  Simulation field;Setup(field,SpellId::WaterArray);Enemy(field,{3,0});field.TestCast(SpellId::WaterArray,5);
  assert(!field.GetSnapshot().effects.empty());const auto spec=GetSpellConfig(SpellId::WaterArray,5);assert(field.GetSnapshot().effects[0].followPlayer);assert(field.GetSnapshot().effects[0].life>=spec.duration);
  field.TestSetPlayer({1,1},50);Advance(field,.05f);bool followed=false;for(auto&e:field.GetSnapshot().effects)if(e.source==SpellId::WaterArray)followed=std::abs(e.position.x-1)<.01f && std::abs(e.position.y-1)<.01f;assert(followed);
  // Beast count creates two distinct orbiting entities which then emit attacks.
  Simulation beast;Setup(beast,SpellId::WoodBeast);Enemy(beast,{6,0});beast.TestCast(SpellId::WoodBeast,5);int companions=0;for(auto&e:beast.GetSnapshot().effects)if(e.form==Form::Beast)++companions;assert(companions==2);
  Advance(beast,.05f);assert(beast.GetSnapshot().effects.size()>=2);auto a=beast.GetSnapshot().effects[0].position,b=beast.GetSnapshot().effects[1].position;assert(std::hypot(a.x-b.x,a.y-b.y)>.1f);
  bool beastShot=false;for(auto&p:beast.GetSnapshot().projectiles)beastShot|=p.form==Form::Beast;assert(beastShot);Advance(beast,1);assert(beast.GetSnapshot().statistics.damageDealt>0 && beast.GetSnapshot().player.hp>50);
  // Talisman chooses a faster distant threat, telegraphs, then explodes there.
  Simulation talisman;Setup(talisman,SpellId::ThunderTalisman);auto slow=Enemy(talisman,{5,0},1),fast=Enemy(talisman,{10,0},4);talisman.TestCast(SpellId::ThunderTalisman,5);
  int warnings=0;for(auto&e:talisman.GetSnapshot().effects)if(e.source==SpellId::ThunderTalisman){assert(e.warning>=.35f);assert(std::abs(e.position.x-10)<.01f);++warnings;}assert(warnings==2);
  for(auto&e:Fixture(talisman).enemies){e.speed=0;}
  Advance(talisman,.2f);assert(Find(talisman,fast).hp==10000);Advance(talisman,.5f);assert(Find(talisman,fast).hp<10000 && Find(talisman,slow).hp==10000);assert(Find(talisman,fast).stun>0);
  // Shield-on-cast is actual survivability, not a descriptive-only field.
  Simulation shield;Setup(shield,SpellId::WaterBeast);shield.TestCast(SpellId::WaterBeast,5);assert(shield.GetSnapshot().player.shield==5);shield.TestDamagePlayer(3);assert(shield.GetSnapshot().player.shield<5);
  // Projectile pierce and rank-five return both reach execution state.
  Simulation sword;Setup(sword,SpellId::MetalSword);Enemy(sword,{6,0});sword.TestCast(SpellId::MetalSword,5);assert(!sword.GetSnapshot().projectiles.empty());assert(sword.GetSnapshot().projectiles[0].pierce==7);assert(sword.GetSnapshot().projectiles[0].returning);
  bool returned=false;for(int i=0;i<150;++i){sword.Step(1.f/60);for(auto&p:sword.GetSnapshot().projectiles)returned|=p.returned;}assert(returned);
  // Shared sword/array rules must apply to the new templates, not only legacy IDs.
  Simulation resonance;Setup(resonance,SpellId::MetalSword);auto& rb=const_cast<ProgressionState&>(resonance.GetProgression().GetState());rb.build.spells.push_back({SpellId::WaterSword,1});
  Relic split;split.catalogId=11;split.rarity=Rarity::Immortal;split.affixes={{AffixKind::Mystery,0,3}};resonance.GetProgression().GrantRelic(split);resonance.RefreshProgression();Enemy(resonance,{3,0});
  for(int n=0;n<5;++n){resonance.TestCast(SpellId::MetalSword,5);}
  assert(resonance.GetSnapshot().projectiles.size()==7);assert(resonance.GetSnapshot().projectiles.front().pierce==8);
  assert(std::abs(resonance.GetSnapshot().projectiles.back().damage/GetSpellConfig(SpellId::MetalSword,5).damage-.6f)<.001f);
  Simulation arrayTwo;Setup(arrayTwo,SpellId::WaterArray);auto& ab=const_cast<ProgressionState&>(arrayTwo.GetProgression().GetState());ab.build.spells.push_back({SpellId::MetalArray,1});arrayTwo.GetProgression().ConfigureFullGame(3);arrayTwo.RefreshProgression();Enemy(arrayTwo,{2,0});arrayTwo.TestCast(SpellId::WaterArray,5);
  assert(std::abs(arrayTwo.GetSnapshot().effects.front().life-GetSpellConfig(SpellId::WaterArray,5).duration*1.25f)<.001f);
  // The projectile actually penetrates two overlapping threats in one forward pass.
  Simulation penetration;Setup(penetration,SpellId::MetalSword,1);auto p1=Enemy(penetration,{3,0}),p2=Enemy(penetration,{3.3f,0});penetration.TestCast(SpellId::MetalSword,1);Advance(penetration,.3f);assert(Find(penetration,p1).hp<10000 && Find(penetration,p2).hp<10000);
  // Water dispels hostile ground fire and gets a documented bonus on burning enemies.
  Simulation water;Setup(water,SpellId::WaterPalm);auto wet=Enemy(water,{2,0});for(auto&e:Fixture(water).enemies)if(e.id==wet)e.burnRemaining=2;
  EffectState flame;flame.id=99999;flame.position={1,0};flame.radius=1;flame.life=4;flame.kind=EffectKind::Burn;flame.hostile=true;Fixture(water).effects.push_back(flame);
  water.TestCast(SpellId::WaterPalm,5);float damage=10000-Find(water,wet).hp;assert(std::abs(damage-GetSpellConfig(SpellId::WaterPalm,5).damage*1.25f)<.01f);Advance(water,.05f);for(auto&e:water.GetSnapshot().effects)assert(e.id!=99999);
  // New passive-route results deal real damage via existing executable templates.
  for(int id=int(SpellId::AscendantPalm);id<int(SpellId::Count);++id){Simulation ascendant;Setup(ascendant,SpellId(id),1);Enemy(ascendant,{2,0});ascendant.TestCast(SpellId(id),1);Advance(ascendant,2);assert(ascendant.GetSnapshot().statistics.damageDealt>0);}
  // Shops and treasure consume the exact same realm-legal pool as ordinary drops and bosses.
  for(int realm=0;realm<3;++realm){Progression pool;pool.Reset(71+realm,HeroId::Body);pool.ConfigureFullGame(3);pool.SetRelicPool(RealmRelicPool(realm));Economy economy;economy.Reset(31+realm,false,true,true);
    for(int rest=1;rest<=30;++rest){pool.BeginRest();assert(economy.BeginRest(pool,rest,true));for(auto&g:economy.Shop())assert(g.relic.catalogId<15 || RelicCatalog()[g.relic.catalogId].realm==realm);
      assert(economy.OpenTreasure(pool,false,true));auto treasure=economy.RevealTreasure(pool);assert(treasure);assert(treasure->catalogId<15 || RelicCatalog()[treasure->catalogId].realm==realm);economy.EndRest(pool);pool.EndRest();}
  }
  std::cout<<"Full spell behaviors: facing, healing, slow/stun, field following, companion count/orbit/projectiles, fastest-target warning/explosion, shield, pierce/return, water-fire interaction passed\n";
}
