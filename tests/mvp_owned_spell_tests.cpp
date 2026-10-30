#include "MVP/Simulation.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace dywx::mvp;

static RunConfig Quiet() {
  RunConfig c;
  c.spawning = false;
  c.formResonance = false;
  return c;
}
static int Rank(const Progression &p, SpellId id) {
  for (auto s : p.GetState().build.spells)
    if (s.id == id)
      return s.rank;
  return 0;
}
// Uses only real earned-upgrade APIs, never mutates a build fixture.
static bool Acquire(Progression &p, SpellId id, int rank) {
  for (int tries = 0; tries < 150; ++tries) {
    if (Rank(p, id) >= rank)
      return true;
    if (p.GetState().offers.empty())
      p.AddXp(p.GetState().xpRequired);
    const auto offers = p.GetState().offers;
    if (offers.empty())
      return false;
    std::size_t pick = 0;
    int best = -1;
    for (std::size_t i = 0; i < offers.size(); ++i) {
      const auto &o = offers[i];
      int score = o.kind == UpgradeKind::ImproveSpell ? 30
                  : o.kind == UpgradeKind::Passive    ? 20
                                                      : 10;
      if (o.kind != UpgradeKind::Passive && o.spell == id)
        score = 100;
      if (score > best) {
        best = score;
        pick = i;
      }
    }
    if (best < 100 && p.Reroll())
      continue;
    assert(p.ChooseUpgrade(pick));
  }
  return false;
}
static Simulation Owned(SpellId id, int rank) {
  for (int seed = 1; seed < 100; ++seed) {
    Simulation s;
    s.Start(seed, HeroId::Body, Quiet());
    if (Acquire(s.GetProgression(), id, rank)) {
      s.RefreshProgression();
      return s;
    }
  }
  assert(false && "No deterministic legal-acquisition fixture found");
  return {};
}
static void Advance(Simulation &s, float seconds) {
  for (int n = 0; n < int(std::ceil(seconds * 60)); ++n)
    s.Step(1.f / 60);
}
static float EnemyHp(const Simulation &s, EntityId id) {
  for (const auto &e : s.GetSnapshot().enemies)
    if (e.id == id)
      return e.hp;
  return 0;
}
static EffectState LastRing(const Simulation &s) {
  for (auto i = s.GetSnapshot().effects.rbegin();
       i != s.GetSnapshot().effects.rend(); ++i)
    if (i->source == SpellId::FlameRing)
      return *i;
  assert(false);
  return {};
}
int main() {
  auto body3 = Owned(SpellId::FlameBody, 3);
  auto body4 = body3;
  assert(Acquire(body4.GetProgression(), SpellId::FlameBody, 4));
  body4.RefreshProgression();
  body3.Step(1.f / 60);
  body4.Step(1.f / 60);
  auto normalizedCooldown = [](const Simulation &s) {
    const auto &v = s.GetSnapshot();
    return v.spellCooldowns[int(SpellId::FlameBody)] /
           (1 - v.progression.build.stats.cooldownReduction);
  };
  assert(std::abs(normalizedCooldown(body3) - .5f) < .0001f);
  assert(std::abs(normalizedCooldown(body4) - .4f) < .0001f);

  auto rank4 = Owned(SpellId::FlameBody, 4);
  auto rank5 = rank4;
  assert(Acquire(rank5.GetProgression(), SpellId::FlameBody, 5));
  rank5.RefreshProgression();
  const auto e4 =
      rank4.TestSpawn(EnemyRole::Support, {2, 0}, EnemyTier::Variant);
  const auto e5 =
      rank5.TestSpawn(EnemyRole::Support, {2, 0}, EnemyTier::Variant);
  rank4.TestDamagePlayer(1);
  rank5.TestDamagePlayer(1);
  assert(std::abs(EnemyHp(rank4, e4) - 120) < .001f);
  assert(std::abs(EnemyHp(rank5, e5) - 80) < .001f);
  rank5.TestDamagePlayer(1);
  assert(std::abs(EnemyHp(rank5, e5) - 80) <
         .001f); // shared six-second cooldown
  rank5.TestDamageEnemy(e5, 10000);
  Advance(rank5, 6.1f);
  const auto next =
      rank5.TestSpawn(EnemyRole::Support, {2, 0}, EnemyTier::Variant);
  rank5.TestDamagePlayer(1);
  assert(std::abs(EnemyHp(rank5, next) - 80) < .001f);

  auto ring3 = Owned(SpellId::FlameRing, 3);
  auto ring4 = ring3;
  assert(Acquire(ring4.GetProgression(), SpellId::FlameRing, 4));
  ring4.RefreshProgression();
  Advance(ring3, .2f);
  Advance(ring4, .2f);
  ring3.TestCast(SpellId::FlameRing, 3);
  ring4.TestCast(SpellId::FlameRing, 4);
  const auto fx3 = LastRing(ring3), fx4 = LastRing(ring4);
  const float t = ring3.GetSnapshot().statistics.elapsed;
  const float cross =
      fx3.position.x * fx4.position.y - fx3.position.y * fx4.position.x;
  const float dot =
      fx3.position.x * fx4.position.x + fx3.position.y * fx4.position.y;
  const float angularDifference = std::atan2(cross, dot);
  assert(std::abs(angularDifference - t * 3 * .25f) < .001f);
  std::cout << "Owned-spell combat tests passed: rank4 aura cadence, rank5 "
               "hurt burst/cooldown, rank4 ring rotation\n";
}
