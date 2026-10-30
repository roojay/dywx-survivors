#include "MVP/Progression.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
using namespace dywx::mvp;
static bool Near(float a, float b) { return std::abs(a - b) < .001f; }
static void Gain(Progression &p, SpellId target, int rank) {
  for (int n = 0; n < 500; ++n) {
    auto &s = p.GetState();
    for (auto sp : s.build.spells)
      if (sp.id == target && sp.rank >= rank)
        return;
    if (s.offers.empty())
      p.AddXp(p.GetState().xpRequired);
    bool found = false;
    for (std::size_t i = 0; i < p.GetState().offers.size(); ++i) {
      auto o = p.GetState().offers[i];
      if (o.kind != UpgradeKind::Passive && o.spell == target) {
        assert(p.ChooseUpgrade(i));
        found = true;
        break;
      }
    }
    if (!found) {
      if (!p.Reroll()) {
        assert(!p.GetState().offers.empty());
        assert(p.ChooseUpgrade(0));
      }
    }
  }
  assert(false);
}
int main() {
  Progression p;
  p.Reset(123, HeroId::Body);
  assert(p.GetState().rerollsRemaining == 2);
  assert(Near(p.GetState().build.stats.maxHp, 120));
  assert(p.GetState().build.resonance.body == 2);
  assert(Near(p.GetState().build.stats.armor, .18f));
  p.AddXp(22);
  assert(p.GetState().offers.empty());
  p.AddXp(1);
  assert(p.GetState().level == 2 && p.GetState().offers.size() == 3);
  assert(Near(p.GetState().xpRequired, 31));
  assert(p.Reroll());
  assert(p.Reroll());
  assert(!p.Reroll());
  p.Reset(123, HeroId::Body);
  assert(p.GetState().rerollsRemaining == 2);
  // Every seed produces distinct legal choices with at least one existing
  // aspect match.
  for (int seed = 0; seed < 200; ++seed) {
    p.Reset(seed, HeroId::Body);
    p.AddXp(23);
    std::set<int> keys;
    bool affinity = false;
    for (auto &o : p.GetState().offers) {
      keys.insert(int(o.kind) * 100 + (o.kind == UpgradeKind::Passive
                                           ? int(o.passive)
                                           : int(o.spell)));
      if (o.kind == UpgradeKind::Passive) {
        assert(o.passive >= PassiveId::FireScripture);
        affinity |= o.passive == PassiveId::FireScripture;
      } else {
        auto s = GetSpellConfig(o.spell);
        affinity |= s.element == Element::Fire || s.form == Form::Body;
      }
    }
    assert(keys.size() == 3 && affinity);
  }
  // All ranks, fixed table changes and 12 entries are executable numeric
  // records.
  for (int id = 0; id < 12; ++id) {
    for (int r = 1; r <= 5; ++r) {
      auto s = GetSpellConfig(SpellId(id), r);
      assert(s.damage > 0 && s.cooldown > 0 && !s.name.empty());
    }
  }
  assert(Near(GetSpellConfig(SpellId::FlamePalm, 2).damage, 19.2f));
  assert(GetSpellConfig(SpellId::FlamePalm, 5).fireAfterwave);
  assert(GetSpellConfig(SpellId::ThunderFist, 5).highestHpChain);
  assert(GetSpellConfig(SpellId::HeavenlyThunder, 5).afterBolts == 2);
  p.Reset(9, HeroId::Body);
  Gain(p, SpellId::FlamePalm, 5);
  Gain(p, SpellId::ThunderFist, 5);
  assert(p.CanFuse(SpellId::FusionPalm));
  assert(!p.CanFuse(SpellId::FusionSwords));
  p.BeginRest();
  auto before = p.GetState().build.resonance;
  auto slots = p.GetState().build.spells.size();
  assert(p.Fuse(SpellId::FusionPalm));
  assert(p.GetState().build.spells.size() == slots - 1);
  auto after = p.GetState().build.resonance;
  assert(before.fire == after.fire && before.thunder == after.thunder &&
         before.body == after.body);
  assert(!p.Fuse(SpellId::FusionPalm));
  p.EndRest();
  // Relic theme, affix counts/ranges, deterministic stream and bag routing.
  Progression a, b;
  a.Reset(777, HeroId::Sword);
  b.Reset(777, HeroId::Sword);
  for (int id = 0; id < 15; ++id) {
    auto x = a.GenerateRelicFromCatalog(id), y = b.GenerateRelicFromCatalog(id);
    assert(x.affixes.size() == (id < 4 ? 2 : id < 9 ? 3 : id < 13 ? 4 : 6));
    assert(x.affixes.size() == y.affixes.size());
    for (std::size_t j = 0; j < x.affixes.size(); ++j) {
      assert(x.affixes[j].kind == y.affixes[j].kind &&
             x.affixes[j].code == y.affixes[j].code &&
             x.affixes[j].value == y.affixes[j].value);
      assert(!DescribeAffix(x.affixes[j]).empty());
    }
  }
  p.Reset(1, HeroId::Body);
  for (int i = 0; i < 8; ++i)
    p.GrantRelic(p.GenerateRelicFromCatalog(i % 4));
  assert(p.GetState().equipped.size() == 6 && p.GetState().bag.size() == 2);
  auto id = p.GetState().bag[0].id;
  assert(!p.Equip(id, 0));
  assert(!p.SetLocked(id, true));
  assert(!p.Unequip(0));
  p.BeginRest();
  assert(p.SetLocked(id, true));
  auto ids = p.FilterBag();
  assert(p.PreviewRefine(ids).ids.size() == 1);
  assert(p.Refine(ids));
  assert(p.PendingRefineXp() == 4);
  assert(p.GetState().bag.size() == 1);
  assert(p.UndoRefine());
  assert(p.PendingRefineXp() == 0 && p.GetState().bag.size() == 2);
  assert(p.SetLocked(id, false));
  assert(p.PreviewRefine({id, id}).xp == 4);
  assert(p.Refine({id}));
  float xp = p.GetState().xp;
  p.EndRest();
  assert(Near(p.GetState().xp, xp + 4));
  assert(!p.UndoRefine());
  p.BeginRest();
  assert(p.Equip(p.GetState().bag.front().id, 0));
  assert(p.Unequip(0));
  p.SortBag();
  assert(p.FilterBag({0, -1, -1, false}).size() == 2);
  p.EndRest();
  // Exclusion applies also to automatic pickups, never deletes or auto-refines.
  p.Reset(1, HeroId::Body);
  p.GrantRelic(p.GenerateRelicFromCatalog(9));
  p.GrantRelic(p.GenerateRelicFromCatalog(10));
  assert(p.GetState().equipped.size() == 1 && p.GetState().bag.size() == 1);
  assert(p.HasMystery(1) && !p.HasMystery(2));
  p.BeginRest();
  assert(!p.Equip(p.GetState().bag[0].id, 1));
  assert(p.Equip(p.GetState().bag[0].id, 0));
  assert(p.HasMystery(2) && !p.HasMystery(1));
  // Shared trigger boolean, unique and special resonance counts do not stack.
  p.Reset(1, HeroId::Body);
  for (int i = 0; i < 2; ++i) {
    Relic r;
    r.id = 100 + i;
    r.affixes = {{AffixKind::Trigger, 0, 4},
                 {AffixKind::Unique, 0, 2},
                 {AffixKind::Resonance, 1, 0}};
    p.GrantRelic(r);
  }
  assert(p.HasTrigger(4));
  assert(p.GetState().build.resonance.fire == 3 &&
         p.GetState().build.resonance.thunder == 1);
  Stats s;
  s.armor = 9;
  s.cooldownReduction = 9;
  s.area = 9;
  s.critChance = 9;
  s.moveSpeed = 99;
  s.pickupRadius = 99;
  s.dashCooldown = -99;
  s = Progression::ClampStats(s);
  assert(Near(s.armor, .6f) && Near(s.cooldownReduction, .5f) && s.area == 2 &&
         Near(s.critChance, .75f) && s.moveSpeed == 8 && s.pickupRadius == 8 &&
         s.dashCooldown == 2);
  assert(EffectiveLuck(0) == 0 && EffectiveLuck(10000) <= 40);
  p.Reset(123, HeroId::Body);
  for (int i = 0; i < 20; ++i)
    p.OnWaveEnded();
  assert(p.PityWaves() == 20);
  auto drop = p.OnKill(EnemyTier::Elite);
  assert(drop && drop->rarity == Rarity::Immortal && p.PityWaves() == 0);
  p.OnWaveEnded();
  assert(p.PityWaves() == 0);
  p.OnWaveEnded();
  assert(p.PityWaves() == 1);
  for (int i = 0; i < 1000; ++i) {
    auto r = p.OnKill(EnemyTier::Elite);
    assert(r && r->rarity != Rarity::Common);
  }
  auto rewards = p.BossRewards();
  std::set<int> rids;
  for (auto &r : rewards) {
    assert(r.rarity >= Rarity::Immortal);
    rids.insert(r.catalogId);
  }
  assert(rids.size() == 3);
  // Long-run progression cannot duplicate owned spells/passives or exceed caps.
  p.Reset(42, HeroId::Sword);
  p.AddXp(100000);
  for (int n = 0; n < 300 && !p.GetState().offers.empty(); ++n)
    assert(p.ChooseUpgrade(0));
  std::set<int> sp, pa;
  for (auto &s : p.GetState().build.spells) {
    assert(s.rank <= 5);
    sp.insert(int(s.id));
  }
  for (auto x : p.GetState().build.passives)
    pa.insert(int(x));
  assert(sp.size() == p.GetState().build.spells.size() && sp.size() <= 6 &&
         pa.size() == p.GetState().build.passives.size() && pa.size() <= 4);
  assert(p.GetState().offers.empty());
  // Exclusive starter never leaks through repeated upgrades/rerolls, including
  // capped builds.
  for (auto hero : {HeroId::Body, HeroId::Sword})
    for (int seed = 0; seed < 20; ++seed) {
      p.Reset(seed, hero);
      p.AddXp(100000);
      for (int n = 0; n < 100 && !p.GetState().offers.empty(); ++n) {
        for (auto &o : p.GetState().offers)
          if (o.kind != UpgradeKind::Passive)
            assert(o.spell != (hero == HeroId::Body ? SpellId::FlameSword
                                                    : SpellId::FlamePalm));
        if (n < 2)
          p.Reroll();
        else
          assert(p.ChooseUpgrade(0));
      }
    }
  p.Reset(9, HeroId::Body);
  p.BeginRest();
  p.GenerateBossRewards();
  assert(p.GetState().bossRewards.size() == 3);
  assert(p.ChooseBossReward(0));
  assert(p.GetState().bossRewards.empty());
  p.GenerateBossRewards();
  assert(p.GetState().bossRewards.empty());
  // Host persistence may restore discoveries, never run power or possessions.
  p.Reset(99, HeroId::Body);
  p.AddXp(500);
  p.GrantRelic(p.GenerateRelicFromCatalog(14));
  p.SetPityWaves(19);
  auto previous = p.GetState();
  previous.seenSpells[11] = true;
  previous.seenFusions[1] = true;
  p.Reset(7, HeroId::Sword);
  const auto fresh = p.GetState();
  p.MergeDiscoveries(previous);
  p.MergeDiscoveries(previous);
  const auto &merged = p.GetState();
  assert(merged.seenRelics[14] && merged.seenSpells[11] &&
         merged.seenFusions[1]);
  assert(merged.seenSpells[int(SpellId::FlameSword)] &&
         merged.seenSpells[int(SpellId::FlamePalm)]);
  assert(merged.level == 1 && merged.xp == 0 && merged.pendingUpgrades == 0 &&
         merged.offers.empty() && merged.equipped.empty() &&
         merged.bag.empty());
  assert(merged.build.spells.size() == 1 &&
         merged.build.spells[0].id == SpellId::FlameSword &&
         merged.build.passives.size() == 1);
  assert(merged.build.stats.power == fresh.build.stats.power &&
         merged.build.stats.maxHp == fresh.build.stats.maxHp &&
         merged.build.resonance.fire == fresh.build.resonance.fire);
  assert(p.PityWaves() == 0 && merged.pityWaves == 0 &&
         merged.rerollsRemaining == 2);
  p.SetPityWaves(previous.pityWaves);
  assert(p.PityWaves() == 19);
  // Statistical quality and drop smoke checks with wide deterministic
  // tolerances.
  p.Reset(8844, HeroId::Body);
  int drops = 0;
  for (int i = 0; i < 100000; ++i)
    if (p.OnKill(EnemyTier::Normal))
      ++drops;
  assert(drops > 380 && drops < 650);
  std::cout << "MVP progression: all assertions passed; normal drops " << drops
            << "/100000\n";
}
