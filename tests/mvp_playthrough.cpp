// Natural calibration uses only keyboard-like inputs and legal progression
// commands.
// --benchmark is a deliberately separate injected stationary-target experiment.
#include "MVP/Simulation.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace dywx::mvp;
static float len(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
static Vec2 unit(Vec2 v) { return v * (1 / std::max(.001f, len(v))); }
static int rank(const BuildState &b, SpellId id) {
  for (auto s : b.spells)
    if (s.id == id)
      return s.rank;
  return 0;
}
static std::string build(const BuildState &b) {
  std::ostringstream o;
  for (auto s : b.spells)
    o << int(s.id) << ":" << s.rank << ";";
  o << "P";
  for (auto p : b.passives)
    o << int(p) << ";";
  return o.str();
}
static int value(const UpgradeOffer &o, const BuildState &b, int policy) {
  auto first =
      b.hero == HeroId::Body ? SpellId::FlamePalm : SpellId::FlameSword;
  auto second =
      b.hero == HeroId::Body ? SpellId::ThunderFist : SpellId::ThunderSword;
  if (o.kind == UpgradeKind::Passive)
    return o.passive == PassiveId::FireScripture ? 65
           : o.passive == PassiveId::ClearMind   ? 35
                                                 : 30;
  int v = 20;
  if (o.spell == SpellId::FlameBody)
    v = o.kind == UpgradeKind::NewSpell ? 110 : 60;
  if (o.spell == SpellId::FlameArray)
    v = o.kind == UpgradeKind::NewSpell ? 90 : 45;
  if (o.spell == SpellId::FlameRing)
    v = o.kind == UpgradeKind::NewSpell ? 85 : 40;
  if (o.spell == first || o.spell == second)
    v = 75 + (rank(b, first) + rank(b, second) >= 6 ? 25 : 0);
  if (o.spell == SpellId::HeavenlyThunder)
    v = 55;
  if (o.spell == SpellId::SwordRain)
    v = 50;
  if (policy == 1 && (o.spell == first || o.spell == second))
    v += 30;
  if (policy == 2 && o.kind == UpgradeKind::NewSpell)
    v += 25;
  return v;
}
static void upgrades(Simulation &s, int policy, std::string &log) {
  auto &p = s.GetProgression();
  auto f = p.GetState().build.hero == HeroId::Body ? SpellId::FusionPalm
                                                   : SpellId::FusionSwords;
  if (p.CanFuse(f) && p.Fuse(f)) {
    log += "F" + std::to_string(int(f)) + ";";
    s.RefreshProgression();
  }
  if (s.GetSnapshot().progression.offers.empty())
    return;
  for (;;) {
    auto ps = s.GetSnapshot().progression;
    size_t pick = 0;
    int best = -1;
    for (size_t i = 0; i < ps.offers.size(); ++i) {
      int v = value(ps.offers[i], ps.build, policy);
      if (v > best) {
        best = v;
        pick = i;
      }
    }
    if (best < 65 && ps.rerollsRemaining > 0) {
      if (!s.Reroll())
        throw std::runtime_error("reroll rejected");
      log += "R;";
      continue;
    }
    auto o = ps.offers[pick];
    log += std::to_string(int(o.kind)) + ":" +
           std::to_string(o.kind == UpgradeKind::Passive ? int(o.passive)
                                                         : int(o.spell)) +
           ";";
    if (!s.ChooseUpgrade(pick))
      throw std::runtime_error("choice rejected");
    break;
  }
}
static float equipmentValue(const Stats &s) {
  return s.maxHp * .05f + s.armor * 35 + s.regen * 15 + s.power * 20 +
         s.cooldownReduction * 45 + s.moveSpeed * 3 + s.area * 8 +
         s.pickupRadius + s.critChance * 12 + (s.triggers[3] ? 15 : 0);
}
static void rest(Simulation &s) {
  auto &p = s.GetProgression();
  for (int pass = 0; pass < 6; ++pass) {
    EntityId best = 0;
    size_t slot = 0;
    float score = equipmentValue(p.GetState().build.stats);
    auto bag = p.GetState().bag;
    for (auto r : bag)
      for (size_t i = 0; i < p.GetState().equipped.size(); ++i) {
        float v = equipmentValue(p.PreviewEquip(r.id, i));
        if (v > score + .01f) {
          score = v;
          best = r.id;
          slot = i;
        }
      }
    if (!best || !p.Equip(best, slot))
      break;
  }
  std::vector<EntityId> ids;
  for (auto &r : p.GetState().bag)
    if (!r.locked)
      ids.push_back(r.id);
  if (!ids.empty())
    p.Refine(ids);
  s.RefreshProgression();
}
static float segment(Vec2 p, Vec2 a, Vec2 b) {
  Vec2 v = b - a;
  float dot = (p.x - a.x) * v.x + (p.y - a.y) * v.y;
  float t = std::clamp(dot / std::max(.001f, v.x * v.x + v.y * v.y), 0.f, 1.f);
  return len(p - (a + v * t));
}
static InputState decide(const Snapshot &s, int policy) {
  Vec2 p = s.player.position;
  const EnemyState *target = nullptr;
  float nearest = 1e6f;
  const EnemyState *boss = nullptr;
  for (auto &e : s.enemies) {
    float d = len(e.position - p);
    if (d < nearest) {
      nearest = d;
      target = &e;
    }
    if (e.role == EnemyRole::Boss)
      boss = &e;
  }
  if (boss)
    target = boss;
  bool melee = s.progression.build.hero == HeroId::Body;
  float desired = boss ? (melee ? 3.f : 6.f) : (melee ? 2.5f : 4.f);
  if (policy == 3 && !melee)
    desired = boss ? 3.2f : 2.9f;
  if (policy == 4 && !melee)
    desired = boss ? 4.f : 3.3f;
  if (rank(s.progression.build, SpellId::FusionPalm))
    desired = 3.4f;
  Vec2 goal = target ? target->position : p;
  float pickupDist = 7;
  if (!boss && nearest > 4)
    for (auto &x : s.pickups) {
      float d = len(x.position - p);
      if (d < pickupDist) {
        pickupDist = d;
        goal = x.position;
      }
    }
  Vec2 circle = unit(Vec2{-p.y / 12, p.x / 22});
  if (len(circle) < .1f)
    circle = {1, 0};
  float best = -1e20f;
  InputState result;
  // Evaluate hold plus 8 keyboard-direction headings. No teleport or separate
  // aim.
  for (int dash = 0; dash < 2; ++dash) {
    if (dash && (s.player.dashCooldown > 0 || nearest > 4))
      continue;
    for (int i = -1; i < 8; ++i) {
      Vec2 move = i < 0 ? Vec2{}
                        : Vec2{std::cos(i * 6.2831853f / 8),
                               std::sin(i * 6.2831853f / 8)};
      float score = 0;
      for (float t : {.15f, .4f, .7f}) {
        Vec2 q = p + move * (dash ? 4.f + s.progression.build.stats.moveSpeed *
                                              std::max(0.f, t - .3f)
                                  : s.progression.build.stats.moveSpeed * t);
        score -= std::max(0.f, std::abs(q.x) - 36) * 6 +
                 std::max(0.f, std::abs(q.y) - 19) * 8;
        for (auto o : GetArenaObstacles())
          score -= std::max(0.f, o.radius + .7f - len(q - o.position)) * 25;
        for (auto &e : s.enemies) {
          if (e.role == EnemyRole::Boss)
            continue;
          Vec2 ep = e.position;
          if (e.role == EnemyRole::Swarm || e.role == EnemyRole::Exploder)
            ep = ep + unit(p - ep) * e.speed * t;
          float d = len(q - ep);
          score -= std::max(0.f, 2.3f - d) * 14;
          if (d < e.radius + .65f)
            score -= 35;
        }
        for (auto &fx : s.effects)
          if (fx.hostile && fx.warning < t + .3f) {
            float d = fx.kind == EffectKind::Line
                          ? segment(q, fx.position, fx.end)
                          : len(q - fx.position);
            if (fx.kind == EffectKind::FireWall) {
              if (std::abs(q.x) > 19 || std::abs(q.y) > 12)
                score -= 70;
            } else
              score -= std::max(0.f, fx.radius + .7f - d) * 35;
          }
        for (auto &pr : s.projectiles)
          if (pr.hostile)
            score -=
                std::max(0.f, .9f - segment(q, pr.position,
                                            pr.position + pr.velocity * t)) *
                30;
        if (target) {
          float d = len(q - goal);
          score -= std::abs(d - desired) * (boss ? 7.f : 1.5f);
        }
      }
      if (!boss)
        score += (move.x * circle.x + move.y * circle.y) * 3;
      // Facing is held when no key is pressed. Read current cooldown cues,
      // never set them.
      Vec2 facing = i < 0 ? s.player.facing : move;
      if (target && melee) {
        Vec2 to = unit(target->position - p);
        float dot = to.x * facing.x + to.y * facing.y;
        for (auto sp : s.progression.build.spells)
          if ((sp.id == SpellId::FlamePalm || sp.id == SpellId::ThunderFist) &&
              s.spellCooldowns[int(sp.id)] < .15f &&
              len(target->position - p) <
                  GetSpellConfig(sp.id, sp.rank).radius *
                          s.progression.build.stats.area +
                      target->radius)
            score += dot > .55f ? 30 : -20;
      }
      if (dash)
        score -= 8;
      if (policy == 2 && i < 0)
        score += 1;
      if (score > best) {
        best = score;
        result = {move, bool(dash)};
      }
    }
  }
  return result;
}
static std::string frame(const Snapshot &s) {
  std::ostringstream o;
  o << "{\"time\":" << s.statistics.elapsed << ",\"phase\":" << int(s.phase)
    << ",\"bossPhase\":" << s.bossPhase
    << ",\"player\":{\"x\":" << s.player.position.x
    << ",\"y\":" << s.player.position.y << ",\"hp\":" << s.player.hp
    << ",\"facingX\":" << s.player.facing.x
    << ",\"facingY\":" << s.player.facing.y << "},\"enemies\":[";
  bool comma = false;
  for (auto &e : s.enemies) {
    if (comma)
      o << ",";
    comma = true;
    o << "{\"id\":" << e.id << ",\"role\":" << int(e.role)
      << ",\"x\":" << e.position.x << ",\"y\":" << e.position.y
      << ",\"radius\":" << e.radius << ",\"hp\":" << e.hp
      << ",\"windup\":" << e.windup << ",\"tier\":" << int(e.tier)
      << ",\"maxHp\":" << e.maxHp << ",\"directionX\":" << e.direction.x
      << ",\"directionY\":" << e.direction.y << "}";
  }
  o << "],\"projectiles\":[";
  comma = false;
  for (auto &e : s.projectiles) {
    if (comma)
      o << ",";
    comma = true;
    o << "{\"id\":" << e.id << ",\"source\":" << int(e.source)
      << ",\"x\":" << e.position.x << ",\"y\":" << e.position.y
      << ",\"radius\":" << e.radius
      << ",\"hostile\":" << (e.hostile ? "true" : "false") << "}";
  }
  o << "],\"effects\":[";
  comma = false;
  for (auto &e : s.effects) {
    if (comma)
      o << ",";
    comma = true;
    o << "{\"id\":" << e.id << ",\"kind\":" << int(e.kind)
      << ",\"source\":" << int(e.source) << ",\"x\":" << e.position.x
      << ",\"y\":" << e.position.y << ",\"endX\":" << e.end.x
      << ",\"endY\":" << e.end.y << ",\"radius\":" << e.radius
      << ",\"warning\":" << e.warning << ",\"life\":" << e.life
      << ",\"duration\":" << e.duration << ",\"element\":" << int(e.element)
      << ",\"hostile\":" << (e.hostile ? "true" : "false") << "}";
  }
  o << "]}";
  return o.str();
}
static void natural(int seed, HeroId hero, int policy, int maxSeconds,
                    const std::string &tracePath = "") {
  std::deque<std::string> trace;
  Simulation s;
  s.Start(seed, hero);
  std::string log;
  float entered = -1, minHp1 = 14000, minHp2 = 16000;
  int peak = 0;
  InputState input;
  for (int tick = 0; tick < maxSeconds * 60; ++tick) {
    const auto &x = s.GetSnapshot();
    if (x.phase == RunPhase::Dead || x.phase == RunPhase::Won)
      break;
    upgrades(s, policy, log);
    if (s.GetSnapshot().phase == RunPhase::Rest) {
      rest(s);
      s.BeginNextWave();
      continue;
    }
    if (tick % 6 == 0)
      input = decide(s.GetSnapshot(), policy);
    s.Step(1.f / 60, input);
    const auto &n = s.GetSnapshot();
    if (!tracePath.empty() && tick % 6 == 0) {
      trace.push_back(frame(n));
      if (trace.size() > 100)
        trace.pop_front();
    }
    peak = std::max(peak, int(n.enemies.size()));
    for (auto &e : n.enemies)
      if (e.role == EnemyRole::Boss) {
        if (entered < 0)
          entered = n.statistics.elapsed;
        if (e.bossPhase == 1)
          minHp1 = std::min(minHp1, e.hp);
        else {
          minHp1 = 0;
          minHp2 = std::min(minHp2, e.hp);
        }
      }
    if (!std::isfinite(n.player.hp) || n.enemies.size() > 160 ||
        n.progression.rerollsRemaining < 0)
      throw std::runtime_error("natural invariant");
  }
  const auto &x = s.GetSnapshot();
  if (!tracePath.empty()) {
    trace.push_back(frame(x));
    if (trace.size() > 100)
      trace.pop_front();
    std::ofstream out(tracePath);
    if (!out)
      throw std::runtime_error("trace open failed");
    out << "{\"schema\":\"dywx-mvp-trace-v1\",\"source\":\"actual pure C++ "
           "Simulation; not UE render\",\"seed\":"
        << seed << ",\"hero\":" << int(hero) << ",\"policy\":" << policy
        << ",\"sampleHz\":10,\"frames\":[";
    bool comma = false;
    for (auto &f : trace) {
      if (comma)
        out << ",";
      comma = true;
      out << f;
    }
    out << "]}\n";
  }
  if (x.phase == RunPhase::Won)
    minHp1 = minHp2 = 0;
  std::cout << "natural," << seed << "," << int(hero) << "," << policy << ","
            << (x.phase == RunPhase::Won    ? "won"
                : x.phase == RunPhase::Dead ? "dead"
                                            : "survived_bound")
            << "," << x.statistics.elapsed << "," << entered << ","
            << x.bossPhase << "," << 30000 - minHp1 - minHp2 << ","
            << x.player.hp << "," << x.statistics.kills << ","
            << x.progression.level << "," << peak << ",\""
            << build(x.progression.build) << "\",\"" << log << "\"\n";
}
static void benchmark() {
  std::cout
      << "mode,seed,hero,policy,upgrades,distance,seconds,damage,dps,build\n";
  for (auto hero : {HeroId::Body, HeroId::Sword})
    for (int seed : {11, 29, 73})
      for (int policy : {0, 1, 2})
        for (int equipment : {0, 1})
          for (float distance : {2.5f, 6.f}) {
            Simulation s;
            RunConfig c;
            c.spawning = false;
            c.waveDurations.fill(1000);
            c.bossHp1 = 100000;
            s.Start(seed, hero, c);
            std::string log;
            // BENCH ONLY: grant exactly enough XP for each of 19 choices;
            // offers remain legal.
            for (int i = 0; i < 19; ++i) {
              const auto &p = s.GetProgression().GetState();
              s.GetProgression().AddXp(
                  (p.xpRequired - p.xp) / p.build.stats.xpMultiplier + .001f);
              s.RefreshProgression();
              upgrades(s, policy, log);
            }
            if (s.GetProgression().GetState().level != 20 ||
                s.GetProgression().GetState().pendingUpgrades != 0)
              throw std::runtime_error(
                  "benchmark did not consume exactly19 upgrades");
            // Fuse only at a legal rest, without spending/inventing an
            // additional upgrade.
            s.GetProgression().BeginRest();
            auto f = hero == HeroId::Body ? SpellId::FusionPalm
                                          : SpellId::FusionSwords;
            if (s.GetProgression().CanFuse(f))
              s.GetProgression().Fuse(f);
            s.GetProgression().EndRest();
            s.RefreshProgression();
            if (equipment) {
              for (int catalog : {4, 5, 6, 7, 8, 4})
                s.GetProgression().GrantRelic(
                    s.GetProgression().GenerateRelicFromCatalog(catalog));
              s.RefreshProgression();
            }
            auto id =
                s.TestSpawn(EnemyRole::Boss, {distance, 0}, EnemyTier::Boss);
            // BENCH ONLY fixture: immobilize one boss and suppress its AI,
            // shields/adds/attacks. No TestDamage is used: all measured damage
            // comes from automatic production casts.
            auto &snap = const_cast<Snapshot &>(s.GetSnapshot());
            for (auto &e : snap.enemies)
              if (e.id == id)
                e.stun = 10000;
            for (int tick = 0; tick < 120 * 60; ++tick)
              s.Step(1.f / 60);
            auto &r = s.GetSnapshot();
            if (r.enemies.size() != 1 || r.phase != RunPhase::Wave ||
                r.enemies.front().hp <= 0 ||
                r.progression.build.spells.size() > 6 ||
                r.progression.build.passives.size() > 4 ||
                r.progression.equipped.size() != size_t(equipment ? 6 : 0))
              throw std::runtime_error("benchmark fixture invariant");
            std::cout << (equipment ? "controlled_six_spirit,"
                                    : "controlled_no_relic,")
                      << seed << "," << int(hero) << "," << policy << ",19,"
                      << distance << ",120," << r.statistics.damageDealt << ","
                      << r.statistics.damageDealt / 120 << ",\""
                      << build(r.progression.build) << "\"\n";
          }
}
int main(int argc, char **argv) {
  if (argc > 2 && std::string(argv[1]) == "--trace") {
    natural(argc > 3 ? std::stoi(argv[3]) : 12,
            argc > 4 ? HeroId(std::stoi(argv[4])) : HeroId::Sword,
            argc > 5 ? std::stoi(argv[5]) : 0, 1800, argv[2]);
    return 0;
  }
  if (argc > 1 && std::string(argv[1]) == "--benchmark") {
    benchmark();
    return 0;
  }
  bool sweep = argc > 1 && std::string(argv[1]) == "--sweep";
  int seconds = !sweep && argc > 1 ? std::stoi(argv[1]) : 1800;
  std::cout << "mode,seed,hero,policy,outcome,elapsed,bossEntered,bossPhase,"
               "bossDamage,hp,kills,level,peakEnemies,build,choices\n";
  if (sweep) {
    int first = argc > 2 ? std::stoi(argv[2]) : 1;
    int count = argc > 3 ? std::stoi(argv[3]) : 24;
    for (int seed = first; seed < first + count; ++seed)
      for (auto hero : {HeroId::Body, HeroId::Sword})
        natural(seed, hero, 0, seconds);
    return 0;
  }
  for (int policy : {0, 1, 2, 3, 4})
    for (auto hero : {HeroId::Body, HeroId::Sword})
      for (int seed : {11, 29, 73})
        natural(seed, hero, policy, seconds);
}
