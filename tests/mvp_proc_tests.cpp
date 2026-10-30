#include "MVP/Simulation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace dywx::mvp;
static RunConfig Quiet() {
  RunConfig c;
  c.spawning = false;
  c.automaticSpells = false;
  return c;
}
static void Advance(Simulation &s, float time) {
  for (int i = 0; i < int(std::ceil(time * 60)); ++i)
    s.Step(1.f / 60);
}
static void Equip(Simulation &s, std::initializer_list<Affix> affixes) {
  Relic r;
  r.id = 900;
  r.catalogId = 0;
  r.affixes = affixes;
  s.GetProgression().GrantRelic(r);
  s.RefreshProgression();
}
static const EnemyState &Enemy(const Simulation &s, EntityId id) {
  for (const auto &e : s.GetSnapshot().enemies)
    if (e.id == id)
      return e;
  std::abort();
}
static void AssertOffersBlocked(Simulation &s) {
  s.GetProgression().AddXp(23);
  s.RefreshProgression();
  const auto before = s.GetSnapshot().progression;
  assert(!before.offers.empty());
  assert(!s.ChooseUpgrade(0));
  assert(!s.Reroll());
  const auto &after = s.GetSnapshot().progression;
  assert(after.pendingUpgrades == before.pendingUpgrades);
  assert(after.rerollsRemaining == before.rerollsRemaining);
  assert(after.seenSpells == before.seenSpells);
  assert(after.build.spells.size() == before.build.spells.size());
}
int main() {
  {
    Simulation title;
    title.GetProgression().Reset(1, HeroId::Body);
    AssertOffersBlocked(title);
    Simulation paused;
    paused.Start(1, HeroId::Body, Quiet());
    paused.Pause(true);
    AssertOffersBlocked(paused);
    paused.Pause(false);
    assert(paused.ChooseUpgrade(0));
    Simulation dead;
    dead.Start(1, HeroId::Body, Quiet());
    dead.TestDamagePlayer(10000);
    AssertOffersBlocked(dead);
  }
  // T3 adds 15 percentage points to power, and chains retain their source form.
  for (bool dash : {false, true}) {
    Simulation s;
    s.Start(1, HeroId::Body, Quiet());
    Equip(s, {{AffixKind::Power, .5f, 0},
              {AffixKind::Crit, -.05f, 0},
              {AffixKind::Trigger, 0, 3},
              {AffixKind::Thunder, .2f, 0}});
    if (dash)
      s.Step(1.f / 60, {{}, true});
    const auto origin = s.GetSnapshot().player.position;
    auto direct = s.TestSpawn(EnemyRole::Support, origin + Vec2{2, 0});
    auto chain = s.TestSpawn(EnemyRole::Support, origin + Vec2{4, 0});
    s.TestCast(SpellId::ThunderFist);
    const float scale = (dash ? 1.65f : 1.5f) * 1.2f * 1.15f;
    assert(std::abs(Enemy(s, direct).hp - (120 - 20 * scale)) < .001f);
    assert(std::abs(Enemy(s, chain).hp - (120 - 12 * scale)) < .001f);
    s.TestCast(SpellId::FlamePalm);
    const float burn = 3 * (dash ? 1.65f : 1.5f) * 1.15f;
    assert(std::abs(Enemy(s, direct).burnDps - burn) < .001f);
  }
  // Projectile chains inherit Sword form bonuses and its timed 4-count burst.
  for (bool burst : {false, true}) {
    Simulation s;
    bool acquired = false;
    for (int seed = 1; seed <= 100 && !acquired; ++seed) {
      s.Start(seed, HeroId::Sword, Quiet());
      auto &progression = s.GetProgression();
      progression.AddXp(23);
      const auto offers = progression.GetState().offers;
      for (std::size_t i = 0; i < offers.size(); ++i)
        if (offers[i].kind == UpgradeKind::NewSpell &&
            offers[i].spell == SpellId::SwordRain) {
          assert(progression.ChooseUpgrade(i));
          acquired = true;
          break;
        }
    }
    assert(acquired);
    Equip(s, {{AffixKind::Resonance, 1, 3},
              {AffixKind::Sword, .2f, 0},
              {AffixKind::Crit, -.05f, 0}});
    assert(s.GetSnapshot().progression.build.resonance.sword == 4);
    if (burst)
      Advance(s, 8.1f);
    auto direct = s.TestSpawn(EnemyRole::Support, {2, 0});
    auto chain = s.TestSpawn(EnemyRole::Support, {4, 0});
    s.TestCast(SpellId::ThunderSword);
    Advance(s, .15f);
    const float scale = 1.08f * 1.2f * (burst ? 1.3f : 1.f);
    assert(std::abs(Enemy(s, direct).hp - (120 - 12 * scale)) < .001f);
    assert(std::abs(Enemy(s, chain).hp - (120 - 6 * scale)) < .001f);
  }
  // Death procs settle in the same tick, including the wave's final tick.
  // Seed 3 deterministically triggers T6 after the first burn kill.
  for (bool finalTick : {false, true}) {
    Simulation s;
    auto config = Quiet();
    config.waveDurations[0] = finalTick ? .1f : 100.f;
    s.Start(3, HeroId::Body, config);
    Equip(s, {{AffixKind::Trigger, 0, 6}});
    if (finalTick)
      for (int i = 0; i < 5; ++i)
        s.Step(1.f / 60);
    auto victim = s.TestSpawn(EnemyRole::Support, {2, 0}, EnemyTier::Variant);
    s.TestCast(SpellId::FlamePalm);
    s.TestDamageEnemy(victim, Enemy(s, victim).hp - .001f);
    auto bystander = s.TestSpawn(EnemyRole::Swarm, {2, 1});
    s.TestDamageEnemy(bystander, 17);
    s.Step(1.f / 60);
    assert(s.GetSnapshot().statistics.kills == 2);
    assert(s.GetSnapshot().enemies.empty());
    if (finalTick) {
      assert(s.GetSnapshot().phase == RunPhase::Rest);
      assert(s.GetSnapshot().progression.xp == 7);
      s.GetProgression().AddXp(23);
      s.RefreshProgression();
      assert(s.Reroll());
      assert(s.ChooseUpgrade(0));
    } else {
      float xp = s.GetSnapshot().progression.xp;
      for (const auto &pickup : s.GetSnapshot().pickups)
        xp += pickup.xp;
      assert(xp == 7);
    }
  }
  // Victory cleanup must not discard another enemy killed in the same batch.
  {
    Simulation s;
    auto config = Quiet();
    config.maxEnemies = 2;
    config.bossHp2 = 80;
    config.waveDurations.fill(.1f);
    s.Start(1, HeroId::Body, config);
    for (int wave = 0; wave < 4; ++wave) {
      s.Step(.2f);
      assert(s.BeginNextWave());
    }
    auto boss = s.GetSnapshot().enemies.front().id;
    s.TestDamageEnemy(boss, 20000);
    const auto enemies = s.GetSnapshot().enemies;
    for (const auto &enemy : enemies)
      if (enemy.id != boss)
        s.TestDamageEnemy(enemy.id, 10000);
    const auto kills = s.GetSnapshot().statistics.kills;
    const auto position = Enemy(s, boss).position;
    s.TestSetPlayer(position, 120);
    assert(s.TestSpawn(EnemyRole::Swarm, position) != 0);
    s.TestCast(SpellId::FusionPalm);
    s.Step(1.f / 60);
    assert(s.GetSnapshot().phase == RunPhase::Won);
    assert(s.GetSnapshot().statistics.kills == kills + 1);
    assert(s.GetSnapshot().progression.xp == 7);
    AssertOffersBlocked(s);
  }
  // A boss charge telegraph must freeze with its stunned owner's windup.
  {
    Simulation s;
    auto config = Quiet();
    config.maxEnemies = 1;
    config.waveDurations.fill(.1f);
    s.Start(1, HeroId::Body, config);
    for (int wave = 0; wave < 4; ++wave) {
      s.Step(.2f);
      assert(s.BeginNextWave());
    }
    assert(s.GetSnapshot().phase == RunPhase::Boss);
    auto boss = s.GetSnapshot().enemies.front().id;
    s.TestDamageEnemy(boss, 20000);
    for (int i = 0; i < 650 && Enemy(s, boss).windup <= 0; ++i)
      s.Step(1.f / 60);
    assert(Enemy(s, boss).windup > 0);
    s.TestSetPlayer(Enemy(s, boss).position, 120);
    s.TestCast(SpellId::FusionPalm);
    Advance(s, 1.5f);
    assert(Enemy(s, boss).windup > 0);
    bool warning = false;
    for (const auto &effect : s.GetSnapshot().effects)
      if (effect.owner == boss && effect.kind == EffectKind::Line &&
          effect.warning > 0)
        warning = true;
    assert(warning);
  }
  bool shield = false, critLightning = false, burnExplosion = false,
       shockwave = false, channel = false;
  for (int seed = 1; seed <= 200; ++seed) {
    // T2: shield appears on a real hit, absorbs damage, cannot retrigger within
    // 8s.
    {
      Simulation s;
      s.Start(seed, HeroId::Body, Quiet());
      Equip(s, {{AffixKind::Trigger, 0, 2}});
      s.TestDamagePlayer(1);
      if (s.GetSnapshot().player.shield > 0) {
        shield = true;
        float before = s.GetSnapshot().player.shield;
        s.TestDamagePlayer(1);
        assert(s.GetSnapshot().player.shield < before);
        Advance(s, 4.1f);
        assert(s.GetSnapshot().player.shield == 0);
      }
    }
    // T5: first critical impact can produce exactly one additional 15-damage
    // hit.
    {
      Simulation s;
      s.Start(seed, HeroId::Body, Quiet());
      Equip(s, {{AffixKind::Crit, .7f, 0}, {AffixKind::Trigger, 0, 5}});
      s.TestSpawn(EnemyRole::Support, {4, 0}, EnemyTier::Variant);
      s.TestCast(SpellId::ThunderSword, 1);
      Advance(s, .35f);
      if (s.GetSnapshot().statistics.hits >= 2)
        critLightning = true;
    }
    // T6: burning victim's death explosion damages a nearby unburned target.
    {
      Simulation s;
      s.Start(seed, HeroId::Body, Quiet());
      Equip(s, {{AffixKind::Trigger, 0, 6}});
      auto victim = s.TestSpawn(EnemyRole::Support, {2, 0}, EnemyTier::Variant);
      s.TestCast(SpellId::FlamePalm);
      auto bystander =
          s.TestSpawn(EnemyRole::Support, {2, 1}, EnemyTier::Variant);
      float before = Enemy(s, bystander).hp;
      s.TestDamageEnemy(victim, 10000);
      if (Enemy(s, bystander).hp < before)
        burnExplosion = true;
    }
    // M5: shockwave is secondary; each cast may add one hit per nearby victim,
    // never recurse.
    {
      Simulation s;
      s.Start(seed, HeroId::Body, Quiet());
      Equip(s, {{AffixKind::Mystery, 0, 5}});
      s.TestSpawn(EnemyRole::Support, {2, 0}, EnemyTier::Variant);
      s.TestCast(SpellId::ThunderFist);
      if (s.GetSnapshot().statistics.hits > 1)
        shockwave = true;
      assert(s.GetSnapshot().statistics.hits <= 2);
    }
    // M6: source cooldown is untouched; a different spell is reduced by exactly
    // .3.
    {
      Simulation a, b;
      a.Start(seed, HeroId::Body, Quiet());
      b.Start(seed, HeroId::Body, Quiet());
      Equip(a, {{AffixKind::Crit, .7f, 0}, {AffixKind::Mystery, 0, 6}});
      Equip(b, {{AffixKind::Crit, .7f, 0}});
      for (auto *s : {&a, &b}) {
        s->TestSpawn(EnemyRole::Support, {4, 0}, EnemyTier::Variant);
        s->TestCast(SpellId::HeavenlyThunder);
        s->TestCast(SpellId::ThunderSword);
        Advance(*s, .35f);
      }
      const auto &ac = a.GetSnapshot().spellCooldowns;
      const auto &bc = b.GetSnapshot().spellCooldowns;
      assert(std::abs(ac[int(SpellId::ThunderSword)] -
                      bc[int(SpellId::ThunderSword)]) < .001f);
      float diff =
          bc[int(SpellId::HeavenlyThunder)] - ac[int(SpellId::HeavenlyThunder)];
      assert(diff < .301f);
      if (diff > .29f)
        channel = true;
    }
  }
  assert(shield && critLightning && burnExplosion && shockwave && channel);
  // U1 reset exactly on the 20s cycle; no reset at 19.9 seconds.
  {
    Simulation s;
    s.Start(123, HeroId::Body, Quiet());
    Equip(s, {{AffixKind::Unique, 0, 1}});
    Advance(s, 19);
    s.TestCast(SpellId::HeavenlyThunder);
    Advance(s, .5f);
    assert(s.GetSnapshot().spellCooldowns[int(SpellId::HeavenlyThunder)] > 11);
    Advance(s, .6f);
    assert(s.GetSnapshot().spellCooldowns[int(SpellId::HeavenlyThunder)] == 0);
  }
  // Cap and terminal regressions: phase change reserves room for the boss;
  // victory is empty.
  {
    Simulation s;
    auto c = Quiet();
    c.waveDurations.fill(.1f);
    s.Start(1, HeroId::Body, c);
    for (int wave = 0; wave < 4; ++wave) {
      Advance(s, .2f);
      assert(s.BeginNextWave());
    }
    EntityId boss = 0;
    for (auto e : s.GetSnapshot().enemies)
      if (e.role == EnemyRole::Boss)
        boss = e.id;
    for (int n = 0; n < 76; ++n)
      s.TestSpawn(EnemyRole::Swarm, {40, 20});
    s.TestDamageEnemy(boss, 100000);
    assert(s.GetSnapshot().enemies.size() <= 80);
    s.TestDamageEnemy(boss, 100000);
    assert(s.GetSnapshot().phase == RunPhase::Won &&
           s.GetSnapshot().enemies.empty());
    assert(s.GetSnapshot().progression.bossRewards.size() == 3);
  }
  std::cout << "mvp_proc_tests: T2/T5/T6/M5/M6/U1 + boss caps passed across "
               "200 fixed seeds\n";
}
