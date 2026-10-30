#include "MVP/Progression.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <tuple>
using namespace dywx::mvp;
int main(){
  static_assert(int(SpellId::FusionPalm)==10 && int(SpellId::FusionSwords)==11,"legacy discovery IDs must remain stable");
  static_assert(int(Element::None)==2 && int(Form::None)==3,"legacy enum IDs");
  int bases=0;std::set<std::string_view> spellNames;
  std::set<Element> elements;std::set<Form> forms;
  for(int i=0;i<int(kSpellCount);++i){auto id=SpellId(i);auto c=GetSpellConfig(id);assert(c.damage>0 && c.cooldown>0 && c.radius>0);assert(spellNames.insert(c.name).second);
    if(!IsFusion(id))++bases;
    elements.insert(c.element);forms.insert(c.form);
    for(int rank=1;rank<=5;++rank){auto r=GetSpellConfig(id,rank);assert(r.damage>0);assert(!DescribeSpell(id,rank).empty());}
    if(i>=12)assert(c.behavior!=SpellBehavior::Legacy && c.provisional);
  }
  assert(bases==30 && elements.size()==6 && forms.size()==5);
  assert(FusionCatalog().size()==6 && PassiveCatalog().size()==11);
  std::set<std::string_view> relicNames;
  std::set<std::tuple<int,int,int,int,int>> themes;
  Progression p;p.Reset(123,HeroId::Beast);p.ConfigureFullGame(2);
  assert(p.GetState().build.spells[0].id==SpellId::WoodBeast);
  assert(p.GetState().build.resonance.beast==2);
  assert(p.GetState().build.stats.formBonus[int(Form::Beast)]>.24f);
  for(const auto&c:RelicCatalog()){
    assert(relicNames.insert(c.name).second);auto r=p.GenerateRelicFromCatalog(c.id);assert(r.catalogId==c.id && r.rarity==c.rarity);
    const int expected=c.rarity==Rarity::Common?2:c.rarity==Rarity::Spirit?3:c.rarity==Rarity::Immortal?4:6;assert(int(r.affixes.size())==expected);
    if(c.id>=15)assert(themes.insert({int(c.rarity),int(c.primary),int(c.aspect),c.trigger,c.mystery}).second);
    assert(!DescribeRelic(c.id).empty());
  }
  assert(relicNames.size()==80);
  Progression locked;locked.Reset(12,HeroId::Body);locked.AddXp(23);assert(!locked.SealOffer(0));assert(!locked.SkipUpgrade());
  p.AddXp(1000);assert(!p.GetState().offers.empty());
  auto offer=p.GetState().offers[0];assert(p.SealOffer(0));assert(p.GetState().sealsRemaining==2);
  for(int j=0;j<30;++j){for(auto o:p.GetState().offers)assert(!(o.kind==offer.kind && (o.kind==UpgradeKind::Passive?o.passive==offer.passive:o.spell==offer.spell)));if(!p.Reroll())break;}
  assert(p.SealOffer(0));assert(p.SealOffer(0));assert(!p.SealOffer(0));
  assert(p.SkipUpgrade());assert(p.TakeSkipStones()==5);assert(p.TakeSkipStones()==0);
  // Test fixture seeds a legal rank-five Beast build; production upgrades reach the same state.
  Progression fusion;fusion.Reset(5,HeroId::Beast);
  auto& fixture=const_cast<ProgressionState&>(fusion.GetState());
  fixture.build.spells={{SpellId::WoodBeast,5},{SpellId::WaterBeast,5}};
  fusion.ConfigureFullGame(3);auto before=fusion.GetState().build.resonance;
  fusion.BeginRest();assert(fusion.CanFuse(SpellId::FusionBeast));assert(fusion.Fuse(SpellId::FusionBeast));
  const auto& fused=fusion.GetState();assert(fused.build.spells.size()==1 && fused.seenFusions[2]);
  assert(fused.build.resonance.wood==before.wood && fused.build.resonance.water==before.water && fused.build.resonance.beast==before.beast);
  Progression resonances;resonances.Reset(5,HeroId::Sword);
  auto& rf=const_cast<ProgressionState&>(resonances.GetState());
  rf.build.spells={{SpellId::MetalPalm,1},{SpellId::MetalSword,1},{SpellId::MetalArray,1},{SpellId::MetalTalisman,1}};
  resonances.ConfigureFullGame(3);assert(resonances.GetState().build.resonance.metal==4);assert(resonances.GetState().build.stats.critChance>.149f);assert(resonances.GetState().seenResonances[5]);
  // Discovery arrays retain every appended ID when merged.
  ProgressionState discoveries;discoveries.seenSpells[32]=true;discoveries.seenRelics[79]=true;discoveries.seenFusions[2]=true;discoveries.seenPassives[10]=true;discoveries.seenResonances[10]=true;
  resonances.MergeDiscoveries(discoveries);assert(resonances.GetState().seenRelics[79] && resonances.GetState().seenSpells[32] && resonances.GetState().seenPassives[10]);
  for(std::size_t i=3;i<FusionCatalog().size();++i){const auto&r=FusionCatalog()[i];Progression route;route.Reset(77,r.hero);
    auto& f=const_cast<ProgressionState&>(route.GetState());f.build.spells={{r.first,5}};f.build.passives={r.passive};route.ConfigureFullGame(3);auto prior=route.GetState().build.resonance;
    route.BeginRest();assert(route.CanFuse(r.result));assert(route.Fuse(r.result));assert(route.GetState().build.spells.size()==1 && route.GetState().build.passives.size()==1);assert(route.GetState().seenFusions[i]);
    auto after=route.GetState().build.resonance;assert(after.fire==prior.fire && after.wood==prior.wood && after.metal==prior.metal && after.body==prior.body && after.sword==prior.sword && after.beast==prior.beast);assert(!route.CanFuse(r.result));
  }
  // Permanent baselines survive recomputation and never alter luck indirectly.
  Stats base;base.maxHp=110;base.power=1.1f;Progression bonus;bonus.Reset(2,HeroId::Sword);bonus.ConfigureFullGame(3,base,1);
  assert(std::abs(bonus.GetState().build.stats.maxHp-110)<.001f);assert(std::abs(bonus.GetState().build.stats.power-1.18f)<.001f);assert(bonus.GetState().rerollsRemaining==3);
  // Full-game drops include the added themes while MVP random sequence remains legacy.
  bool newDrop=false;for(int i=0;i<100;++i)newDrop|=p.GenerateRelic(Rarity::Spirit).catalogId>=15;assert(newDrop);
  for(int i=0;i<int(kResonanceCount);++i)assert(!DescribeResonance(i).empty());
  p.SetContentUnlocks(false,false);for(int i=0;i<100;++i)assert(p.GenerateRelic(Rarity::Spirit).catalogId<15);p.SetContentUnlocks(true,true);
  p.GenerateBossRewards(1);auto bossId=p.GetState().bossRewards[0].id;p.GenerateBossRewards(1);assert(p.GetState().bossRewards[0].id==bossId);p.GenerateBossRewards(2);assert(p.GetState().bossRewards[0].id!=bossId);
  p.SetDropRiskWeight(2);p.SetRelicPool({16,18,29});
  Progression lowRisk,highRisk;lowRisk.Reset(51,HeroId::Body);highRisk.Reset(51,HeroId::Body);highRisk.SetDropRiskWeight(4);
  int lowRare=0,highRare=0;for(int n=0;n<10000;++n){lowRare+=lowRisk.OnKill(EnemyTier::Elite)->rarity>=Rarity::Immortal;highRare+=highRisk.OnKill(EnemyTier::Elite)->rarity>=Rarity::Immortal;}assert(highRare>lowRare*2);
  Progression tierOne;tierOne.Reset(3,HeroId::Beast);tierOne.ConfigureFullGame(1);assert(std::abs(tierOne.GetState().build.stats.formBonus[int(Form::Beast)]-.1f)<.001f);tierOne.SetSystemLayer(2);assert(tierOne.GetState().build.stats.formBonus[int(Form::Beast)]>.24f);
  // Holding a recipe scripture measurably tilts its spell half without changing the group caps.
  int plainOffers=0,scriptureOffers=0;for(int seed=1;seed<=600;++seed){for(int held=0;held<2;++held){Progression tilted;tilted.Reset(seed,HeroId::Body);if(held)const_cast<ProgressionState&>(tilted.GetState()).build.passives.push_back(PassiveId::FireScripture);tilted.ConfigureFullGame(3);tilted.AddXp(23);for(auto o:tilted.GetState().offers)if(o.kind!=UpgradeKind::Passive && o.spell==SpellId::FlamePalm){if(held)++scriptureOffers;else ++plainOffers;}}}assert(scriptureOffers>plainOffers);
  // Gradual immortal protection is full-game-only; hard pity still forces the next random item.
  Progression gradual;gradual.Reset(7,HeroId::Body);auto old0=gradual.DropRarityProbabilities();gradual.SetPityWaves(10);assert(gradual.DropRarityProbabilities()==old0);
  gradual.ConfigureFullGame(3);gradual.SetPityWaves(0);auto p0=gradual.DropRarityProbabilities();gradual.SetPityWaves(10);auto p10=gradual.DropRarityProbabilities();gradual.SetPityWaves(19);auto p19=gradual.DropRarityProbabilities();assert(p10[2]>p0[2] && p19[2]>p10[2]);
  gradual.SetPityWaves(20);assert(gradual.DropRarityProbabilities()[2]==1);assert(gradual.OnKill(EnemyTier::Elite)->rarity==Rarity::Immortal);assert(gradual.PityWaves()==0);assert(!gradual.DescribeDropOdds().empty());
  // All rarity sources share the realm's legal catalog; the old fifteen remain shared.
  std::set<int> realmItems;for(int realm=0;realm<3;++realm){Progression pool;pool.Reset(101+realm,HeroId::Body);pool.ConfigureFullGame(3);auto ids=RealmRelicPool(realm);assert(!ids.empty());pool.SetRelicPool(ids);
    for(int id:ids){assert(id>=15 && RelicCatalog()[id].realm==realm);assert(realmItems.insert(id).second);}
    for(int rarity=0;rarity<4;++rarity)for(int n=0;n<100;++n){auto r=pool.GenerateRelic(Rarity(rarity));assert(r.catalogId<15 || RelicCatalog()[r.catalogId].realm==realm);}
    for(int n=0;n<100;++n){auto rewards=pool.BossRewards();std::set<int> unique;for(auto r:rewards){assert(r.catalogId<15 || RelicCatalog()[r.catalogId].realm==realm);unique.insert(r.catalogId);}assert(unique.size()==3);}
  }assert(realmItems.size()==65);
  std::cout<<"Full content: 30 executable base configs, 6 fusions, 80 unique themes; sealing/skip/permanent baseline passed\n";
}
