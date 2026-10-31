#include "MVP/Simulation.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
using namespace dywx::mvp;

static RunConfig Quiet() {
  RunConfig c; c.spawning=false; c.automaticSpells=false;
  c.waveDurations.fill(.1f); return c;
}
static ProgressionState& Fixture(Simulation& s) {
  return const_cast<ProgressionState&>(s.GetProgression().GetState());
}
// Stable semantic digest: no object-padding or container-address comparisons.
static std::string Digest(const ProgressionState& p) {
  std::ostringstream out;
  out<<p.xp<<':'<<p.level<<':'<<p.pendingUpgrades<<':'<<p.rerollsRemaining
     <<':'<<p.xpRequired<<':'<<p.pendingRefineXp<<':'<<p.build.stats.maxHp
     <<':'<<p.build.stats.power<<':'<<p.build.stats.armor;
  for(const auto* v:{&p.equipped,&p.bag}) {
    out<<'|';
    for(const auto& r:*v) {
      out<<r.id<<','<<r.catalogId<<','<<int(r.rarity)<<','<<r.locked<<','<<r.carryOut<<';';
      for(const auto& a:r.affixes) out<<int(a.kind)<<','<<a.value<<','<<a.code<<';';
    }
  }
  for(const auto& s:p.build.spells)out<<'s'<<int(s.id)<<','<<s.rank;
  for(auto passive:p.build.passives)out<<'p'<<int(passive);
  for(const auto& o:p.offers)out<<'o'<<int(o.kind)<<','<<int(o.spell)<<','<<int(o.passive);
  for(bool seen:p.seenSpells)out<<seen;
  for(bool seen:p.seenFusions)out<<seen;
  return out.str();
}
static void Synced(const Simulation& s) {
  assert(Digest(s.GetSnapshot().progression)==Digest(s.GetProgression().GetState()));
}
template<class F> static void Rejected(Simulation& s, F command) {
  const auto before=Digest(s.GetProgression().GetState());
  const auto snapshot=Digest(s.GetSnapshot().progression);
  const auto hp=s.GetSnapshot().player.hp;
  assert(!command());
  assert(before==Digest(s.GetProgression().GetState()));
  assert(snapshot==Digest(s.GetSnapshot().progression));
  assert(hp==s.GetSnapshot().player.hp);
}
static void SeedInventory(Simulation& s) {
  // GrantRelic auto-equips six; the last two enter the bag in unsorted order.
  for(unsigned i=1;i<=8;++i) {
    Relic r; r.id=i; r.catalogId=0;
    r.rarity=i==8?Rarity::Spirit:Rarity::Common;
    r.affixes={{AffixKind::MaxHp,float(i),0}};
    s.GetProgression().GrantRelic(r);
  }
  s.RefreshProgression();
}
static void SeedFusion(Simulation& s, const FusionRecipe& r) {
  auto& p=Fixture(s);
  p.build.spells={{r.first,5}};p.build.passives.clear();
  if(r.passive==PassiveId::Count)p.build.spells.push_back({r.second,5});
  else p.build.passives.push_back(r.passive);
  s.RefreshProgression();
}
int main() {
  Simulation s; s.Start(11,HeroId::Body,Quiet());SeedInventory(s);
  // Even an internally-resting progression cannot bypass the run phase boundary.
  s.GetProgression().BeginRest();
  for(auto phase:{RunPhase::Title,RunPhase::Wave,RunPhase::Boss,RunPhase::Won,RunPhase::Dead}) {
    const_cast<Snapshot&>(s.GetSnapshot()).phase=phase;
    Rejected(s,[&]{return s.Equip(7,0);});
    Rejected(s,[&]{return s.Unequip(0);});
    Rejected(s,[&]{return s.SetLocked(7,true);});
    Rejected(s,[&]{return s.SortBag();});
  }
  s.Start(11,HeroId::Body,Quiet()); SeedInventory(s);s.Step(.2f);
  assert(s.GetSnapshot().phase==RunPhase::Rest);
  s.Pause(true);
  Rejected(s,[&]{return s.Equip(7,0);});
  Rejected(s,[&]{return s.Unequip(0);});
  Rejected(s,[&]{return s.SetLocked(7,true);});
  Rejected(s,[&]{return s.SortBag();});
  s.Pause(false);
  Rejected(s,[&]{return s.Equip(999,0);});
  Rejected(s,[&]{return s.Equip(7,6);});
  Rejected(s,[&]{return s.Unequip(6);});
  Rejected(s,[&]{return s.SetLocked(999,true);});
  assert(s.SortBag());Synced(s);assert(s.GetSnapshot().progression.bag[0].id==8);
  assert(s.SetLocked(8,true));Synced(s);assert(s.GetSnapshot().progression.bag[0].locked);
  assert(s.Equip(8,0));Synced(s);assert(s.GetSnapshot().progression.equipped[0].id==8);
  assert(s.SetLocked(8,false));Synced(s);assert(!s.GetSnapshot().progression.equipped[0].locked);
  s.TestSetPlayer({},s.GetSnapshot().progression.build.stats.maxHp);
  assert(s.Unequip(0));Synced(s);
  assert(s.GetSnapshot().player.hp==s.GetSnapshot().progression.build.stats.maxHp);
  assert(s.Equip(8,5));Synced(s);

  Simulation slots;slots.Start(29,HeroId::Body,Quiet());slots.Step(.2f);
  assert(slots.SortBag());Synced(slots); // Empty sorting is an accepted no-op.
  Relic first;first.id=101;first.affixes={{AffixKind::Mystery,0,1}};
  Relic conflicting;conflicting.id=102;conflicting.affixes={{AffixKind::Mystery,0,2}};
  slots.GetProgression().GrantRelic(first);slots.GetProgression().GrantRelic(conflicting);
  slots.RefreshProgression();
  Rejected(slots,[&]{return slots.Equip(102,2);}); // No gaps in equipped slots.
  Rejected(slots,[&]{return slots.Equip(102,1);}); // Mutually exclusive mysteries.
  assert(slots.Equip(102,0));Synced(slots); // Replacement removes the conflict.

  // Every recipe remains available under its existing hero/rank/passive rules.
  for(const auto& r:FusionCatalog()) {
    Simulation f;f.Start(17,r.hero,Quiet());SeedFusion(f,r);
    assert(f.GetProgression().CanFuse(r.result));
    assert(!f.CanFuse(r.result));Rejected(f,[&]{return f.Fuse(r.result);});
    f.Step(.2f);assert(f.GetSnapshot().phase==RunPhase::Rest);
    f.Pause(true);assert(!f.CanFuse(r.result));Rejected(f,[&]{return f.Fuse(r.result);});
    f.Pause(false);
    Fixture(f).build.spells[0].rank=4;f.RefreshProgression();
    assert(!f.CanFuse(r.result));Rejected(f,[&]{return f.Fuse(r.result);});
    SeedFusion(f,r);
    if(r.passive!=PassiveId::Count) {
      Fixture(f).build.passives.clear();f.RefreshProgression();
      Rejected(f,[&]{return f.Fuse(r.result);});SeedFusion(f,r);
    } else {
      Fixture(f).build.spells.pop_back();f.RefreshProgression();
      Rejected(f,[&]{return f.Fuse(r.result);});SeedFusion(f,r);
    }
    assert(f.CanFuse(r.result));assert(f.Fuse(r.result));Synced(f);
    assert(f.GetSnapshot().progression.build.spells.size()==1);
    assert(f.GetSnapshot().progression.build.spells[0].id==r.result);
    assert(!f.CanFuse(r.result));Rejected(f,[&]{return f.Fuse(r.result);});
  }
  const auto& recipe=FusionCatalog()[0];
  for(auto phase:{RunPhase::Wave,RunPhase::Boss,RunPhase::Title,RunPhase::Won,RunPhase::Dead}) {
    Simulation f;f.Start(23,recipe.hero,Quiet());SeedFusion(f,recipe);
    f.GetProgression().AddXp(23);f.RefreshProgression();
    assert(!f.GetSnapshot().progression.offers.empty());
    const_cast<Snapshot&>(f.GetSnapshot()).phase=phase;
    if(phase==RunPhase::Wave || phase==RunPhase::Boss) {
      const auto pending=f.GetSnapshot().progression.pendingUpgrades;
      assert(f.CanFuse(recipe.result));assert(f.Fuse(recipe.result));Synced(f);
      assert(f.GetSnapshot().progression.pendingUpgrades==pending);
    } else {assert(!f.CanFuse(recipe.result));Rejected(f,[&]{return f.Fuse(recipe.result);});}
  }
  Simulation wrongHero;wrongHero.Start(3,HeroId::Sword,Quiet());SeedFusion(wrongHero,recipe);
  wrongHero.Step(.2f);Rejected(wrongHero,[&]{return wrongHero.Fuse(recipe.result);});
  Rejected(wrongHero,[&]{return wrongHero.Fuse(SpellId::Count);});
  std::cout<<"core command regressions passed\n";
}
