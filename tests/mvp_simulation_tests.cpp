#include "MVP/Simulation.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
static RunConfig Quiet() {
  RunConfig c;
  c.spawning = false;
  return c;
}
static void Advance(Simulation &s, float seconds, InputState input = {}) {
  int ticks = int(std::ceil(seconds * 60));
  for (int i = 0; i < ticks; ++i)
    s.Step(1.f / 60, input);
}
static float Dist(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }
int main() {
  {
    Simulation s;
    s.Start(7, HeroId::Body, Quiet());
    assert(s.GetSnapshot().waveCount == 4);
    assert(s.GetSnapshot().waveDuration == 100);
    assert(s.GetSnapshot().player.hp == 120);
    assert(s.GetSnapshot().progression.build.stats.dashCooldown == 5.5f);
    Advance(s, 1, {{1, 1}, false});
    assert(std::abs(Dist(s.GetSnapshot().player.position, {}) - 5) < .001f);
  }
  {
    Simulation s;
    s.Start(7, HeroId::Body, Quiet());
    s.Step(1.f / 120, {{1, 0}, true});
    s.Step(1.f / 120, {{1, 0}, false});
    assert(s.GetSnapshot().player.dashRemaining > 0);
    float hp = s.GetSnapshot().player.hp;
    s.TestDamagePlayer(100);
    assert(s.GetSnapshot().player.hp == hp);
    Advance(s, 17.f / 60);
    assert(std::abs(s.GetSnapshot().player.position.x - 4) < .001);
    Advance(s, .1f);
    s.TestDamagePlayer(20);
    assert(s.GetSnapshot().player.hp < hp);
  }
  {
    Simulation s;
    s.Start(2, HeroId::Sword, Quiet());
    Advance(s, 1);
    s.Pause(true);
    auto tick = s.GetSnapshot().tick;
    Advance(s, 2, {{1, 0}, true});
    assert(s.GetSnapshot().tick == tick);
    s.Pause(false);
    s.TestDamagePlayer(100000);
    assert(s.GetSnapshot().phase == RunPhase::Dead);
    Advance(s, 1);
    assert(s.GetSnapshot().tick == tick);
    auto generation = s.GetSnapshot().generation;
    s.Restart();
    assert(s.GetSnapshot().generation == generation + 1);
    assert(s.GetSnapshot().statistics.kills == 0 &&
           s.GetSnapshot().enemies.empty());
  }
  {
    Simulation a, b;
    a.Start(123, HeroId::Sword);
    b.Start(123, HeroId::Sword);
    for (int i = 0; i < 1200; ++i) {
      InputState input{{i < 600 ? 1.f : -1.f, 0}, i == 20};
      a.Step(1.f / 60, input);
      b.Step(1.f / 120, input);
      b.Step(1.f / 120, input);
    }
    auto &x = a.GetSnapshot();
    auto &y = b.GetSnapshot();
    assert(x.tick == y.tick);
    assert(x.enemies.size() == y.enemies.size());
    assert(x.player.position.x == y.player.position.x);
    assert(x.player.hp == y.player.hp);
    assert(x.statistics.kills == y.statistics.kills);
    for (size_t i = 0; i < x.enemies.size(); ++i)
      assert(x.enemies[i].position.x == y.enemies[i].position.x);
  }
  {
    Simulation s;
    auto c = Quiet();
    c.waveDurations.fill(.2f);
    c.bossHp1 = 100;
    c.bossHp2 = 200;
    s.Start(1, HeroId::Sword, c);
    for (int w = 1; w <= 4; ++w) {
      s.TestSpawn(EnemyRole::Swarm, {40, 20});
      Advance(s, .3f);
      assert(s.GetSnapshot().phase == RunPhase::Rest);
      assert(s.GetSnapshot().enemies.empty() &&
             s.GetSnapshot().projectiles.empty() &&
             s.GetSnapshot().effects.empty());
      auto tick = s.GetSnapshot().tick;
      Advance(s, 1);
      assert(s.GetSnapshot().tick == tick);
      assert(s.BeginNextWave());
    }
    assert(s.GetSnapshot().phase == RunPhase::Boss);
    EntityId boss = 0;
    for (auto e : s.GetSnapshot().enemies)
      if (e.role == EnemyRole::Boss)
        boss = e.id;
    s.TestDamageEnemy(boss, 100000);
    assert(s.GetSnapshot().bossPhase == 2 &&
           s.GetSnapshot().phase == RunPhase::Boss);
    s.TestDamageEnemy(boss, 100000);
    assert(s.GetSnapshot().phase == RunPhase::Won);
    assert(s.GetSnapshot().enemies.empty());
  }
  {
    Simulation s;
    s.Start(1, HeroId::Body, Quiet());
    s.TestSpawn(EnemyRole::Charger, {-8, 0}, EnemyTier::Variant);
    Advance(s, 1.1f);
    bool warning = false;
    for (auto e : s.GetSnapshot().effects)
      if (e.hostile && e.warning > 0)
        warning = true;
    assert(warning);
    float hp = s.GetSnapshot().player.hp;
    Advance(s, .6f);
    assert(s.GetSnapshot().player.hp == hp);
    Advance(s, 1);
    assert(s.GetSnapshot().statistics.damageTaken > 0);
  }
  {
    Simulation s;
    s.Start(1, HeroId::Sword, Quiet());
    auto id = s.TestSpawn(EnemyRole::Support, {6, 0}, EnemyTier::Variant);
    Advance(s, .6f);
    bool hit = false;
    for (auto e : s.GetSnapshot().enemies)
      if (e.id == id)
        hit = e.hp < e.maxHp;
    assert(hit);
    assert(s.GetSnapshot().statistics.hits > 0);
  }
  {
    Simulation s;
    auto c = Quiet();
    c.maxEnemies = 4;
    c.maxProjectiles = 3;
    c.maxEffects = 3;
    c.maxPickups = 2;
    s.Start(5, HeroId::Sword, c);
    for (int i = 0; i < 100; ++i) {
      s.TestSpawn(EnemyRole::Support, {float(i % 3 + 3), 0});
      s.TestCast(SpellId::SwordRain, 5);
      s.TestCast(SpellId::FlameSword, 5);
    }
    assert(s.GetSnapshot().enemies.size() <= 4 &&
           s.GetSnapshot().effects.size() <= 3 &&
           s.GetSnapshot().projectiles.size() <= 3);
    Advance(s, 2);
    assert(s.GetSnapshot().statistics.droppedEvents > 0);
    s.Step(std::numeric_limits<float>::quiet_NaN());
    s.Step(-1);
    assert(std::isfinite(s.GetSnapshot().player.hp));
  }
  {
    Simulation s;
    s.Start(1, HeroId::Body, Quiet());
    s.GetProgression().AddXp(100);
    s.RefreshProgression();
    assert(!s.GetSnapshot().progression.offers.empty());
    auto tick = s.GetSnapshot().tick;
    Advance(s, .2f, {{1, 0}, false});
    assert(s.GetSnapshot().tick > tick);
    assert(s.ChooseUpgrade(0));
  }
  {
    Simulation s;
    s.Start(1, HeroId::Body, Quiet());
    s.TestSetPlayer({46, 0}, 120);
    Advance(s, 4, {{1, 0}, false});
    assert(s.GetSnapshot().player.position.x <= 47.55f);
    s.TestSetPlayer({-21, -9}, 120);
    Advance(s, 1, {{1, 0}, false});
    assert(Dist(s.GetSnapshot().player.position, {-18, -9}) >= 2.049f);
  }
  std::cout << "mvp_simulation_tests: 10 suites passed\n";
}
