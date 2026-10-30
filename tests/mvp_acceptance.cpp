#include "MVP/Simulation.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace dywx::mvp;
static int checks = 0;
#define CHECK(x)                                                               \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x))                                                                  \
      throw std::runtime_error(std::string(__func__) + ":" +                   \
                               std::to_string(__LINE__) + " " #x);             \
  } while (0)
static void advance(Simulation &s, float seconds, float fps = 60) {
  for (int n = 0; n < int(seconds * fps + .5f); ++n)
    s.Step(1 / fps);
}
static RunConfig quiet() {
  RunConfig c;
  c.spawning = false;
  c.automaticSpells = false;
  return c;
}
static void finite(const Snapshot &s, const RunConfig &c) {
  CHECK(std::isfinite(s.player.hp));
  CHECK(std::isfinite(s.player.position.x));
  CHECK(std::isfinite(s.player.position.y));
  CHECK(s.enemies.size() <= c.maxEnemies);
  CHECK(s.effects.size() <= c.maxEffects);
  CHECK(s.projectiles.size() <= c.maxProjectiles);
  CHECK(s.pickups.size() <= c.maxPickups);
  CHECK(s.progression.equipped.size() <= 6);
  CHECK(s.progression.build.spells.size() <= 6);
  CHECK(s.progression.build.passives.size() <= 4);
  for (auto &e : s.enemies) {
    CHECK(std::isfinite(e.hp));
    CHECK(std::isfinite(e.position.x));
    CHECK(std::abs(e.position.x) <= 48);
    CHECK(std::abs(e.position.y) <= 27);
  }
}
static void defaults() {
  RunConfig a, b = RunConfig::SixWave();
  CHECK(a.waveDurations[0] == 100 && a.waveDurations[3] == 160);
  CHECK(b.waveDurations[0] == 70 && b.waveDurations[5] == 95);
  CHECK(a.bossHp1 == 14000 && a.bossHp2 == 16000);
  CHECK(a.formResonance);
  for (auto h : {HeroId::Body, HeroId::Sword}) {
    Simulation s;
    s.Start(21, h, quiet());
    auto &p = s.GetSnapshot().progression;
    CHECK(p.build.spells.size() == 1);
    CHECK(p.build.passives.size() == 1);
    CHECK(p.rerollsRemaining == 2);
    CHECK(p.xpRequired == 23);
    CHECK(p.build.spells[0].id ==
          (h == HeroId::Body ? SpellId::FlamePalm : SpellId::FlameSword));
  }
}
static void xpAndQueue() {
  Simulation s;
  s.Start(42, HeroId::Body, quiet());
  auto &p = s.GetProgression();
  p.AddXp(22);
  CHECK(p.GetState().pendingUpgrades == 0);
  p.AddXp(1);
  CHECK(p.GetState().pendingUpgrades == 1);
  CHECK(p.GetState().xpRequired == 31);
  p.AddXp(31 + 39);
  CHECK(p.GetState().pendingUpgrades == 3);
  s.RefreshProgression();
  CHECK(s.GetSnapshot().progression.offers.size() == 3);
  auto t = s.GetSnapshot().tick;
  advance(s, 1);
  CHECK(s.GetSnapshot().tick > t);
  CHECK(s.GetSnapshot().progression.pendingUpgrades == 3);
  CHECK(s.Reroll());
  CHECK(s.Reroll());
  CHECK(!s.Reroll());
  CHECK(!s.ChooseUpgrade(999));
  CHECK(s.ChooseUpgrade(0));
  CHECK(s.GetSnapshot().progression.pendingUpgrades == 2);
}
static void freezeRestart() {
  Simulation s;
  auto c = quiet();
  c.waveDurations.fill(.2f);
  s.Start(19, HeroId::Body, c);
  advance(s, 1);
  CHECK(s.GetSnapshot().phase == RunPhase::Rest);
  auto t = s.GetSnapshot().tick;
  advance(s, 5);
  CHECK(s.GetSnapshot().tick == t);
  CHECK(s.BeginNextWave());
  CHECK(!s.BeginNextWave());
  s.Pause(true);
  t = s.GetSnapshot().tick;
  advance(s, 1);
  CHECK(s.GetSnapshot().tick == t);
  s.Pause(false);
  s.Step(-1);
  s.Step(std::numeric_limits<float>::quiet_NaN());
  CHECK(s.GetSnapshot().tick == t);
  auto gen = s.GetSnapshot().generation;
  s.Restart();
  CHECK(s.GetSnapshot().generation == gen + 1);
  CHECK(s.GetSnapshot().tick == 0);
  CHECK(s.GetSnapshot().enemies.empty());
  CHECK(s.GetSnapshot().progression.pendingUpgrades == 0);
  CHECK(!s.ChooseUpgrade(0));
  CHECK(!s.BeginNextWave());
  s.TestDamagePlayer(1e6f);
  CHECK(s.GetSnapshot().phase == RunPhase::Dead);
  t = s.GetSnapshot().tick;
  advance(s, 1);
  CHECK(s.GetSnapshot().tick == t);
  s.Restart();
  CHECK(s.GetSnapshot().phase == RunPhase::Wave);
}
static void lifecycle() {
  for (bool six : {false, true})
    for (auto hero : {HeroId::Body, HeroId::Sword}) {
      Simulation s;
      auto c = six ? RunConfig::SixWave() : quiet();
      c.spawning = false;
      c.waveDurations.fill(.1f);
      s.Start(67, hero, c);
      for (int w = 1; w <= (six ? 6 : 4); ++w) {
        CHECK(s.GetSnapshot().wave == w);
        advance(s, .2f);
        CHECK(s.GetSnapshot().phase == RunPhase::Rest);
        CHECK(s.BeginNextWave());
      }
      CHECK(s.GetSnapshot().phase == RunPhase::Boss);
      EntityId boss = 0;
      for (auto &e : s.GetSnapshot().enemies)
        if (e.role == EnemyRole::Boss) {
          boss = e.id;
          CHECK(e.hp == 14000);
        }
      CHECK(boss);
      s.TestDamageEnemy(boss, 1e6f);
      CHECK(s.GetSnapshot().bossPhase == 2);
      CHECK(s.GetSnapshot().phase == RunPhase::Boss);
      for (auto &e : s.GetSnapshot().enemies)
        if (e.role == EnemyRole::Boss)
          CHECK(e.hp == 16000);
      s.TestDamageEnemy(boss, 1e6f);
      CHECK(s.GetSnapshot().phase == RunPhase::Won);
      auto t = s.GetSnapshot().tick;
      advance(s, 1);
      CHECK(s.GetSnapshot().tick == t);
      s.Restart();
      CHECK(s.GetSnapshot().bossPhase == 0);
    }
}
static void dash() {
  Simulation s;
  s.Start(31, HeroId::Body, quiet());
  float hp = s.GetSnapshot().player.hp;
  s.Step(1.f / 60, {{1, 0}, true});
  s.TestDamagePlayer(1000);
  CHECK(s.GetSnapshot().player.hp == hp);
  for (int i = 1; i < 18; ++i)
    s.Step(1.f / 60, {{1, 0}, true});
  CHECK(std::abs(s.GetSnapshot().player.position.x - 4) < .02f);
  CHECK(s.GetSnapshot().player.dashCooldown > 5);
  advance(s, .1f);
  s.TestDamagePlayer(10);
  CHECK(s.GetSnapshot().player.hp < hp);
}
static void inventory() {
  Progression p;
  p.Reset(88, HeroId::Sword);
  for (int i = 0; i < 15; ++i)
    p.GrantRelic(p.GenerateRelicFromCatalog(i));
  CHECK(p.GetState().equipped.size() == 6);
  CHECK(p.GetState().bag.size() == 9);
  auto id = p.GetState().bag.back().id;
  CHECK(!p.Equip(id, 0));
  p.BeginRest();
  CHECK(p.SetLocked(id, true));
  CHECK(p.PreviewRefine({id, id, 999999}).xp == 0);
  CHECK(!p.Refine({id}));
  CHECK(p.SetLocked(id, false));
  int value = p.PreviewRefine({id, id}).xp;
  CHECK(value > 0);
  float xp = p.GetState().xp;
  auto size = p.GetState().bag.size();
  CHECK(p.Refine({id, id}));
  CHECK(p.PendingRefineXp() == value);
  CHECK(p.GetState().xp == xp);
  CHECK(p.GetState().bag.size() == size - 1);
  CHECK(!p.Refine({id}));
  CHECK(p.UndoRefine());
  CHECK(p.PendingRefineXp() == 0);
  CHECK(p.GetState().bag.size() == size);
  CHECK(p.Refine({id}));
  p.EndRest();
  CHECK(p.PendingRefineXp() == 0);
  CHECK(!p.UndoRefine());
  int level = p.GetState().level;
  p.EndRest();
  CHECK(p.GetState().level == level);
}
static void partition() {
  Simulation a, b, c;
  a.Start(123, HeroId::Sword, quiet());
  b.Start(123, HeroId::Sword, quiet());
  c.Start(123, HeroId::Sword, quiet());
  advance(a, 10, 30);
  advance(b, 10, 60);
  advance(c, 10, 120);
  CHECK(a.GetSnapshot().tick == b.GetSnapshot().tick);
  CHECK(b.GetSnapshot().tick == c.GetSnapshot().tick);
  CHECK(a.GetSnapshot().statistics.elapsed ==
        c.GetSnapshot().statistics.elapsed);
}
static void safety() {
  Simulation s;
  s.Start(123, HeroId::Body);
  advance(s, 4.9f);
  CHECK(s.GetSnapshot().enemies.empty());
  advance(s, 1);
  CHECK(!s.GetSnapshot().enemies.empty());
  auto c = quiet();
  c.maxEnemies = 40;
  c.maxEffects = 32;
  c.maxProjectiles = 32;
  c.maxPickups = 16;
  s.Start(3, HeroId::Sword, c);
  for (int n = 0; n < 1000; ++n)
    s.TestSpawn(EnemyRole::Summoner, {20, 20});
  CHECK(s.GetSnapshot().enemies.size() == 40);
  for (int n = 0; n < 1200; ++n) {
    s.Step(1.f / 60, {{-1, -1}, n % 180 == 0});
    finite(s.GetSnapshot(), s.GetConfig());
  }
  CHECK(s.GetSnapshot().statistics.droppedEvents > 0);
}

static void effectConsumers() {
  // A constructed single-affix relic isolates production effect consumption; no
  // mock simulation.
  auto equip = [](Simulation &s, AffixKind kind, int code) {
    Relic r;
    r.id = 9000 + code;
    r.affixes.push_back({kind, 0, code});
    s.GetProgression().GrantRelic(r);
    s.RefreshProgression();
  };
  {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    equip(s, AffixKind::Mystery, 1);
    s.Step(1.f / 60, {{1, 0}, true});
    bool fire = false;
    for (auto &e : s.GetSnapshot().effects)
      if (e.kind == EffectKind::Burn && e.damage > 0) {
        fire = true;
        CHECK(e.life > 2.9f);
        CHECK(e.radius == 2);
      }
    CHECK(fire);
  }
  {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    equip(s, AffixKind::Mystery, 4);
    s.TestCast(SpellId::FlameArray, 1);
    CHECK(!s.GetSnapshot().effects.empty());
    auto &e = s.GetSnapshot().effects.back();
    CHECK(e.followPlayer);
    CHECK(std::abs(e.radius - 2.4f) < .001f);
  }
  {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    equip(s, AffixKind::Unique, 2);
    CHECK(s.GetSnapshot().progression.build.resonance.fire == 2);
    CHECK(s.GetSnapshot().progression.build.resonance.thunder == 1);
  }
  {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    auto a = s.TestSpawn(EnemyRole::Exploder, {15, 0});
    auto b = s.TestSpawn(EnemyRole::Summoner, {16, 0});
    float hp = s.GetSnapshot().player.hp;
    s.TestDamageEnemy(a, 1, Element::Thunder);
    CHECK(s.GetSnapshot().player.hp == hp);
    bool found = false;
    for (auto &e : s.GetSnapshot().enemies)
      if (e.id == b) {
        found = true;
        CHECK(e.hp == 135);
      }
    CHECK(found);
  }
  {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    s.TestSpawn(EnemyRole::Summoner, {3, 0});
    s.TestCast(SpellId::HeavenlyThunder, 5);
    CHECK(s.GetSnapshot().effects.size() == 3);
    CHECK(s.GetSnapshot().effects[0].warning == 1.5f);
    CHECK(s.GetSnapshot().effects[1].damage ==
          s.GetSnapshot().effects[0].damage * .4f);
  }
  {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    s.TestCast(SpellId::SwordRain, 1);
    CHECK(s.GetSnapshot().effects.size() == 8);
    for (auto &e : s.GetSnapshot().effects)
      CHECK(e.warning == .8f);
  }
  for (int spell = 0; spell < 12; ++spell) {
    Simulation s;
    s.Start(5, HeroId::Body, quiet());
    s.TestSpawn(EnemyRole::Summoner, {2, 0});
    s.TestCast(SpellId(spell), 5);
    CHECK(!s.GetSnapshot().effects.empty() ||
          !s.GetSnapshot().projectiles.empty() ||
          s.GetSnapshot().statistics.damageDealt > 0);
    finite(s.GetSnapshot(), s.GetConfig());
  }
}

static Relic oneEffect(AffixKind kind, int code) {
  Relic r;
  r.id = 9000 + int(kind) * 100 + code;
  r.affixes.push_back({kind, 0, code});
  return r;
}
static void triggerConsumers() {
  auto setup = [](Simulation &s, AffixKind kind, int code) {
    s.Start(101, HeroId::Body, quiet());
    s.GetProgression().GrantRelic(oneEffect(kind, code));
    s.RefreshProgression();
  };
  {
    Simulation a, b;
    setup(a, AffixKind::Trigger, 1);
    b.Start(101, HeroId::Body, quiet());
    auto id = a.TestSpawn(EnemyRole::Swarm, {20, 0});
    a.TestDamageEnemy(id, 100);
    for (int k = 0; k < 60; ++k) {
      a.Step(1.f / 60, {{1, 0}, false});
      b.Step(1.f / 60, {{1, 0}, false});
    }
    CHECK(a.GetSnapshot().player.position.x >
          b.GetSnapshot().player.position.x * 1.14f);
  }
  {
    Simulation a, b;
    setup(a, AffixKind::Trigger, 3);
    b.Start(101, HeroId::Body, quiet());
    a.Step(1.f / 60, {{1, 0}, true});
    b.Step(1.f / 60, {{1, 0}, true});
    a.TestSpawn(EnemyRole::Summoner, {2, 0});
    b.TestSpawn(EnemyRole::Summoner, {2, 0});
    a.TestCast(SpellId::FlamePalm, 1);
    b.TestCast(SpellId::FlamePalm, 1);
    CHECK(a.GetSnapshot().statistics.damageDealt >
          b.GetSnapshot().statistics.damageDealt * 1.14f);
  }
  {
    Simulation s;
    setup(s, AffixKind::Trigger, 4);
    s.TestDamagePlayer(60);
    float hp = s.GetSnapshot().player.hp;
    for (int k = 0; k < 25; ++k) {
      auto id = s.TestSpawn(EnemyRole::Swarm, {20, 0});
      s.TestDamageEnemy(id, 100);
    }
    CHECK(std::abs(s.GetSnapshot().player.hp - hp - 6) < .01f);
  }
  {
    Simulation a, b;
    setup(a, AffixKind::Trigger, 7);
    b.Start(101, HeroId::Body, quiet());
    a.TestSetPlayer({0, 0}, 30);
    b.TestSetPlayer({0, 0}, 30);
    a.Step(1.f / 60);
    b.Step(1.f / 60);
    float beforeA = a.GetSnapshot().player.hp,
          beforeB = b.GetSnapshot().player.hp;
    a.TestDamagePlayer(10);
    b.TestDamagePlayer(10);
    CHECK((beforeA - a.GetSnapshot().player.hp) <
          (beforeB - b.GetSnapshot().player.hp));
  }
  {
    Simulation s;
    setup(s, AffixKind::Mystery, 2);
    auto id = s.TestSpawn(EnemyRole::Summoner, {4, 0});
    s.Step(1.f / 60, {{1, 0}, true});
    advance(s, .4f);
    bool hit = false;
    for (auto &e : s.GetSnapshot().enemies)
      if (e.id == id)
        hit = e.hp < 160;
    CHECK(hit);
  }
  {
    Simulation s;
    setup(s, AffixKind::Mystery, 3);
    s.TestSpawn(EnemyRole::Summoner, {10, 0});
    for (int k = 0; k < 4; ++k)
      s.TestCast(SpellId::FlameSword, 1);
    CHECK(s.GetSnapshot().projectiles.size() == 4);
    s.TestCast(SpellId::FlameSword, 1);
    CHECK(s.GetSnapshot().projectiles.size() == 7);
  }
}
static void fixedInputPartitions() {
  Simulation a, b, c;
  a.Start(37, HeroId::Sword);
  b.Start(37, HeroId::Sword);
  c.Start(37, HeroId::Sword);
  advance(a, 12, 30);
  advance(b, 12, 60);
  advance(c, 12, 120);
  CHECK(a.GetSnapshot().tick == b.GetSnapshot().tick);
  CHECK(b.GetSnapshot().tick == c.GetSnapshot().tick);
  CHECK(a.GetSnapshot().enemies.size() == b.GetSnapshot().enemies.size());
  CHECK(b.GetSnapshot().statistics.damageDealt ==
        c.GetSnapshot().statistics.damageDealt);
  for (std::size_t i = 0; i < a.GetSnapshot().enemies.size(); ++i) {
    CHECK(a.GetSnapshot().enemies[i].position.x ==
          c.GetSnapshot().enemies[i].position.x);
    CHECK(a.GetSnapshot().enemies[i].hp == c.GetSnapshot().enemies[i].hp);
  }
}

static void bossPopulationCap() {
  Simulation s;
  auto c = quiet();
  c.waveDurations.fill(.1f);
  s.Start(99, HeroId::Body, c);
  for (int w = 0; w < 4; ++w) {
    advance(s, .2f);
    CHECK(s.BeginNextWave());
  }
  EntityId boss = 0;
  for (auto &e : s.GetSnapshot().enemies)
    if (e.role == EnemyRole::Boss)
      boss = e.id;
  while (s.GetSnapshot().enemies.size() < 79)
    s.TestSpawn(EnemyRole::Swarm, {40, 20});
  s.TestDamageEnemy(boss, 1e6f);
  CHECK(s.GetSnapshot().enemies.size() <= 80);
}

static void rankRuntime() {
  for (int spell = 0; spell < 10; ++spell) {
    std::array<Snapshot, 5> ranks;
    for (int rank = 1; rank <= 5; ++rank) {
      Simulation s;
      s.Start(222, HeroId::Body, quiet());
      s.TestSpawn(EnemyRole::Summoner, {2, 0});
      s.TestSpawn(EnemyRole::Summoner, {4, 1});
      s.TestCast(SpellId(spell), rank);
      ranks[rank - 1] = s.GetSnapshot();
      CHECK(!ranks[rank - 1].effects.empty() ||
            !ranks[rank - 1].projectiles.empty());
      finite(ranks[rank - 1], s.GetConfig());
    }
    auto &a = ranks[0];
    auto &b = ranks[1];
    auto &c = ranks[2];
    auto &d = ranks[3];
    auto &e = ranks[4];
    switch (SpellId(spell)) {
    case SpellId::FlamePalm:
      CHECK(b.statistics.damageDealt > a.statistics.damageDealt);
      CHECK(d.enemies[0].burnRemaining == 5);
      CHECK(e.effects.size() > d.effects.size());
      break;
    case SpellId::ThunderFist:
      CHECK(b.statistics.damageDealt > a.statistics.damageDealt);
      break; // chain routing covered by combat owner's multi-target suite
    case SpellId::FlameBody:
      CHECK(b.statistics.damageDealt > a.statistics.damageDealt);
      CHECK(c.effects[0].radius > a.effects[0].radius);
      break; // rank4 cadence and rank5 hurt response require owned-build tests
    case SpellId::FlameSword:
      CHECK(b.projectiles[0].damage > a.projectiles[0].damage);
      CHECK(c.projectiles.size() == 2);
      CHECK(d.projectiles[0].pierce > c.projectiles[0].pierce);
      CHECK(e.projectiles[0].returning);
      break;
    case SpellId::FlameRing:
      CHECK(c.effects.size() > a.effects.size());
      {
        Simulation later;
        later.Start(222, HeroId::Body, quiet());
        advance(later, 6.1f);
        later.TestCast(SpellId::FlameRing, 5);
        const auto &ring = later.GetSnapshot().effects.front();
        CHECK(std::abs(std::sqrt(ring.position.x * ring.position.x +
                                 ring.position.y * ring.position.y) -
                       5) < .01f);
      }
      break; // rotation is observed after advancing time, not t=0
    case SpellId::ThunderSword:
      CHECK(c.projectiles[0].chains > a.projectiles[0].chains);
      CHECK(d.projectiles[0].turnMultiplier == 1.5f);
      CHECK(e.projectiles.size() == 2);
      CHECK(e.projectiles[0].target != e.projectiles[1].target);
      break;
    case SpellId::SwordRain:
      CHECK(a.effects.size() == 8);
      CHECK(c.effects.size() == 12);
      CHECK(e.effects.size() == 13);
      CHECK(e.effects.back().life == 2);
      break;
    case SpellId::FlameArray:
      CHECK(c.effects[0].radius > a.effects[0].radius);
      CHECK(d.effects[0].life == 7);
      CHECK(e.effects[0].followPlayer);
      break;
    case SpellId::ThunderPrison:
      CHECK(c.effects[0].stun == .9f);
      CHECK(d.effects[0].radius == 4.5f);
      CHECK(e.effects[0].prisonVulnerability);
      break;
    case SpellId::HeavenlyThunder:
      CHECK(a.effects[0].warning == 2);
      CHECK(c.effects[0].warning == 1.5f);
      CHECK(e.effects.size() == 3);
      break;
    default:
      break;
    }
  }
}
static void meleeFrontTiming() {
  auto c = quiet();
  c.automaticSpells = true;
  Simulation s;
  s.Start(22, HeroId::Body, c);
  s.TestSpawn(EnemyRole::Summoner, {2, 0});
  s.Step(1.f / 60);
  CHECK(s.GetSnapshot().statistics.damageDealt >= 16);
  Simulation entered;
  entered.Start(22, HeroId::Body, c);
  advance(entered, .1f);
  entered.TestSpawn(EnemyRole::Swarm, {2.5f, 0});
  advance(entered, .7f);
  CHECK(entered.GetSnapshot().statistics.damageDealt >= 18);
  CHECK(entered.GetSnapshot().statistics.damageTaken == 0);
  std::cout << "MELEE post-empty-cast 0.7s enemy-front: dealt="
            << entered.GetSnapshot().statistics.damageDealt
            << " taken=" << entered.GetSnapshot().statistics.damageTaken
            << "\n";
}
static void sustainedStress() {
  Simulation s;
  auto c = quiet();
  c.waveDurations.fill(900);
  c.maxEnemies = 160;
  c.maxEffects = 128;
  c.maxProjectiles = 128;
  c.maxPickups = 128;
  s.Start(991, HeroId::Sword, c);
  auto start = std::chrono::steady_clock::now();
  std::size_t high = 0;
  for (int tick = 0; tick < 36000; ++tick) {
    if (tick % 60 == 0) {
      s.TestSetPlayer({0, 0}, s.GetSnapshot().progression.build.stats.maxHp);
      while (s.GetSnapshot().enemies.size() < 150)
        s.TestSpawn(EnemyRole::Summoner,
                    {20, float(int(s.GetSnapshot().enemies.size()) % 30) - 15});
      for (int spell = 0; spell < 12; ++spell)
        s.TestCast(SpellId(spell), 5);
      finite(s.GetSnapshot(), s.GetConfig());
    }
    s.Step(1.f / 60);
    high = std::max(high, s.GetSnapshot().enemies.size());
    CHECK(s.GetSnapshot().phase == RunPhase::Wave);
  }
  CHECK(high >= 150);
  std::cout << "STRESS seed=991 ticks=36000 simulated_seconds=600 maxEnemies="
            << high
            << " cappedEvents=" << s.GetSnapshot().statistics.droppedEvents
            << " wall_ms="
            << std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - start)
                   .count()
            << " (explicit harness HP resets; not balance)\n";
}
static void autoplay(int tactical = 0) {
  std::cout << "seed,hero,fps,phase,wave,elapsed,kills,level,damageTaken,"
               "maxEnemies,bossPhase,bossHp,damageDealt,wallMs\n";
  for (auto hero : {HeroId::Body, HeroId::Sword})
    for (int seed : {11, 29, 73})
      for (int fps : {30, 60, 120}) {
        Simulation s;
        s.Start(seed, hero);
        std::size_t high = 0;
        float nextAttack = 0;
        EntityId lastSlash = 0;
        auto start = std::chrono::steady_clock::now();
        for (int step = 0; step < fps * 850; ++step) {
          const auto &snap = s.GetSnapshot();
          if (snap.phase == RunPhase::Dead || snap.phase == RunPhase::Won)
            break;
          if (snap.phase == RunPhase::Rest) {
            auto &p = s.GetProgression();
            std::vector<EntityId> ids;
            for (const auto &r : p.GetState().bag)
              ids.push_back(r.id);
            for (auto id : ids) {
              const Relic *item = nullptr;
              for (const auto &r : p.GetState().bag)
                if (r.id == id)
                  item = &r;
              if (!item)
                continue;
              for (std::size_t slot = 0; slot < p.GetState().equipped.size();
                   ++slot)
                if (int(item->rarity) >
                    int(p.GetState().equipped[slot].rarity)) {
                  if (p.Equip(id, slot))
                    break;
                }
            }
            s.RefreshProgression();
            s.BeginNextWave();
            continue;
          }
          if (!snap.progression.offers.empty()) {
            std::size_t pick = 0;
            int score = -1;
            for (std::size_t i = 0; i < snap.progression.offers.size(); ++i) {
              auto &o = snap.progression.offers[i];
              int value = o.kind == UpgradeKind::Fusion         ? 100
                          : o.kind == UpgradeKind::ImproveSpell ? 20
                          : o.kind == UpgradeKind::Passive      ? 10
                                                                : 15;
              if (tactical) {
                if (o.kind == UpgradeKind::NewSpell) {
                  if (o.spell == SpellId::FlameBody)
                    value = 90;
                  else if (o.spell == SpellId::FlameRing)
                    value = 85;
                  else if (o.spell == SpellId::ThunderFist)
                    value = 80;
                  else if (o.spell == SpellId::FlameArray)
                    value = 75;
                  else
                    value = 25;
                }
                if (o.kind == UpgradeKind::ImproveSpell) {
                  value = 40;
                  if (o.spell == SpellId::FlameBody)
                    value = 65;
                  if (o.spell == SpellId::FlamePalm ||
                      o.spell == SpellId::ThunderFist)
                    value = 55;
                }
                if (o.kind == UpgradeKind::Passive)
                  value = o.passive == PassiveId::FireScripture ? 50 : 30;
              }
              if (value > score) {
                score = value;
                pick = i;
              }
            }
            s.ChooseUpgrade(pick);
          }
          const auto &now = s.GetSnapshot();
          Vec2 pos = now.player.position;
          Vec2 move{-pos.y / 12, pos.x / 22};
          float len = std::sqrt(move.x * move.x + move.y * move.y);
          if (len < .01f)
            move = {1, 0};
          else
            move = move * (1 / len);
          float nearest = 100;
          for (const auto &e : now.enemies) {
            Vec2 away = pos - e.position;
            float d = std::sqrt(away.x * away.x + away.y * away.y);
            nearest = std::min(nearest, d);
            if (d < 5 && d > .01f)
              move = move + away * (3 * (5 - d) / (d * 5));
          }
          if (std::abs(pos.x) > 32)
            move.x -= pos.x * .15f;
          if (std::abs(pos.y) > 17)
            move.y -= pos.y * .2f;
          len = std::sqrt(move.x * move.x + move.y * move.y);
          if (len > .01f)
            move = move * (1 / len);
          Vec2 kiteMove = move;
          if (tactical) {
            const EnemyState *target = nullptr;
            float dist = 1e9f;
            for (const auto &enemy : now.enemies) {
              float dx = enemy.position.x - pos.x,
                    dy = enemy.position.y - pos.y;
              float d = std::sqrt(dx * dx + dy * dy);
              if (d < dist) {
                dist = d;
                target = &enemy;
              }
            }
            if (target) {
              move = target->position - pos;
              float range = hero == HeroId::Body ? 2.5f : 6.f;
              if (dist < range && dist > 1.3f)
                move = move * (.04f / std::max(.01f, dist));
              else if (dist < 1.3f && now.player.dashCooldown > 0)
                move = move * (-1 / std::max(.01f, dist));
            } else
              move = {};
            const PickupState *pickup = nullptr;
            float pickupDist = 8;
            for (const auto &item : now.pickups) {
              float dx = item.position.x - pos.x, dy = item.position.y - pos.y;
              float d = std::sqrt(dx * dx + dy * dy);
              if (d < pickupDist) {
                pickupDist = d;
                pickup = &item;
              }
            }
            if (pickup && nearest > 2.5f)
              move = pickup->position - pos;
            if (nearest < 1.3f && now.player.dashCooldown <= 0 && target)
              move = target->position - pos;
            if (tactical >= 2) {
              float cd =
                  GetSpellConfig(now.progression.build.spells.front().id,
                                 now.progression.build.spells.front().rank)
                      .cooldown *
                  (1 - now.progression.build.stats.cooldownReduction);
              for (const auto &fx : now.effects)
                if (fx.kind == EffectKind::Slash && fx.radius >= 3 &&
                    fx.id > lastSlash) {
                  lastSlash = fx.id;
                  nextAttack = now.statistics.elapsed + cd;
                }
              bool turnIn =
                  hero == HeroId::Body &&
                  (tactical == 3 ? now.statistics.elapsed >= nextAttack - .12f
                                 : std::fmod(now.statistics.elapsed,
                                             std::max(.1f, cd)) < .22f) &&
                  dist < 3.2f;
              if (!turnIn)
                move = kiteMove;
              if (pickup && nearest > 4)
                move = pickup->position - pos;
              if (dist > 7 && hero == HeroId::Body && target)
                move = target->position - pos;
            }
          }
          s.Step(1.f / fps, {move, nearest < (tactical ? 1.3f : 2.f) &&
                                       now.player.dashCooldown <= 0});
          high = std::max(high, s.GetSnapshot().enemies.size());
          if (step % fps == 0)
            finite(s.GetSnapshot(), s.GetConfig());
        }
        const auto &r = s.GetSnapshot();
        float bossHp = 0;
        for (const auto &e : r.enemies)
          if (e.role == EnemyRole::Boss)
            bossHp = e.hp;
        std::cout << seed << "," << int(hero) << "," << fps << ","
                  << int(r.phase) << "," << r.wave << ","
                  << r.statistics.elapsed << "," << r.statistics.kills << ","
                  << r.progression.level << "," << r.statistics.damageTaken
                  << "," << high << "," << r.bossPhase << "," << bossHp << ","
                  << r.statistics.damageDealt << ","
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start)
                         .count()
                  << "\n";
      }
}
static void subframeDash() {
  Simulation s;
  s.Start(77, HeroId::Body, quiet());
  s.Step(1.f / 240, {{1, 0}, true});
  for (int i = 0; i < 3; ++i)
    s.Step(1.f / 240, {{1, 0}, false});
  CHECK(s.GetSnapshot().tick == 1);
  CHECK(s.GetSnapshot().player.dashCooldown > 0);
  CHECK(s.GetSnapshot().player.invulnerable > 0);
}
static void warningSaturation() {
  Simulation s;
  auto c = quiet();
  c.maxEffects = 1;
  s.Start(123, HeroId::Body, c);
  s.TestSpawn(EnemyRole::Charger, {8, 0});
  advance(s, 1.05f);
  CHECK(s.GetSnapshot().effects.size() == 1);
  CHECK(s.GetSnapshot().effects[0].hostile);
  CHECK(s.GetSnapshot().effects[0].warning > 0);
  s.TestSpawn(EnemyRole::Exploder, {2, 0});
  advance(s, .85f);
  CHECK(s.GetSnapshot().statistics.damageTaken == 0);
  CHECK(s.GetSnapshot().effects.size() <= 1);
}
int main(int argc, char **argv) {
  try {
    auto start = std::chrono::steady_clock::now();
    if (argc > 1 && (std::string(argv[1]) == "--autoplay" ||
                     std::string(argv[1]) == "--tactical" ||
                     std::string(argv[1]) == "--pulse" ||
                     std::string(argv[1]) == "--observant")) {
      autoplay(std::string(argv[1]) == "--observant"  ? 3
               : std::string(argv[1]) == "--pulse"    ? 2
               : std::string(argv[1]) == "--tactical" ? 1
                                                      : 0);
      return 0;
    }
    subframeDash();
    warningSaturation();
    meleeFrontTiming();
    rankRuntime();
    bossPopulationCap();
    sustainedStress();
    defaults();
    effectConsumers();
    triggerConsumers();
    fixedInputPartitions();
    xpAndQueue();
    freezeRestart();
    lifecycle();
    dash();
    inventory();
    partition();
    safety();
    std::cout << "PASS " << checks << " checks; elapsed_ms="
              << std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - start)
                     .count()
              << "\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << " after " << checks << " checks\n";
    return 1;
  }
}
