#include "MVP/Simulation.h"
#include <cassert>
#include <iostream>
using namespace dywx::mvp;
int main() {
  Progression p, control;
  p.Reset(71, HeroId::Body);
  control.Reset(71, HeroId::Body);
  assert(p.GetState().seenPassives[int(PassiveId::BodyScripture)]);
  assert(!p.GetState().seenPassives[int(PassiveId::SwordScripture)]);
  assert(p.GetState().seenResonances[2]); // Starter plus body scripture.
  for (int i = 0; i < 8; ++i)
    p.RecordEncounter(EnemyRole(i), 1 << (i % 4));
  p.RecordEncounter(EnemyRole(-1), 15);
  p.RecordEncounter(EnemyRole(100), 15);
  const auto discovered = p.GetState();
  for (bool flag : discovered.seenEnemies)
    assert(flag);
  for (bool flag : discovered.seenMutations)
    assert(flag);
  // Recording/merging cannot consume RNG or alter generated relics/offers.
  for (int i = 0; i < 30; ++i) {
    auto a = p.GenerateRelic(Rarity::Immortal);
    auto b = control.GenerateRelic(Rarity::Immortal);
    assert(a.catalogId == b.catalogId && a.affixes.size() == b.affixes.size());
    for (std::size_t j = 0; j < a.affixes.size(); ++j)
      assert(a.affixes[j].kind == b.affixes[j].kind &&
             a.affixes[j].value == b.affixes[j].value &&
             a.affixes[j].code == b.affixes[j].code);
  }
  p.Reset(3, HeroId::Sword, false);
  for (bool flag : p.GetState().seenEnemies)
    assert(!flag);
  for (bool flag : p.GetState().seenMutations)
    assert(!flag);
  for (bool flag : p.GetState().seenResonances)
    assert(!flag);
  const auto fresh = p.GetState();
  p.MergeDiscoveries(discovered);
  p.MergeDiscoveries(discovered);
  for (bool flag : p.GetState().seenEnemies)
    assert(flag);
  for (bool flag : p.GetState().seenMutations)
    assert(flag);
  assert(p.GetState().seenPassives[int(PassiveId::BodyScripture)]);
  assert(p.GetState().seenPassives[int(PassiveId::SwordScripture)]);
  assert(p.GetState().seenResonances[2]);
  assert(p.GetState().build.stats.power == fresh.build.stats.power);
  assert(p.GetState().build.stats.armor == fresh.build.stats.armor);
  assert(p.GetState().build.spells.size() == 1 && p.GetState().bag.empty());
  assert(p.GetState().level == 1 && p.GetState().xp == 0 && p.PityWaves() == 0);
  // Successful arena entry, not proximity or kill, records an encounter.
  RunConfig config;
  config.spawning = config.automaticSpells = false;
  config.maxEnemies = 1;
  Simulation sim;
  sim.Start(19, HeroId::Body, config);
  assert(sim.TestSpawn(EnemyRole::Summoner, {30, 20}, EnemyTier::Elite));
  auto state = sim.GetProgression().GetState();
  assert(state.seenEnemies[int(EnemyRole::Summoner)]);
  int mask = sim.GetSnapshot().enemies.front().mutations;
  for (int i = 0; i < 4; ++i)
    assert(state.seenMutations[i] == bool(mask & (1 << i)));
  assert(!sim.TestSpawn(EnemyRole::Boss, {20, 10}, EnemyTier::Boss));
  assert(!sim.GetProgression().GetState().seenEnemies[int(EnemyRole::Boss)]);
  std::cout << "MVP discoveries: metadata, reset, merge, RNG and encounter "
               "assertions passed\n";
}
