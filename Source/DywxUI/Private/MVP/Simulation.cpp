#include "MVP/Simulation.h"
#include "MVP/ContentCatalog.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace dywx::mvp {
namespace {
constexpr float Pi = 3.14159265359f;
float Len2(Vec2 a) { return a.x * a.x + a.y * a.y; }
float Len(Vec2 a) { return std::sqrt(Len2(a)); }
Vec2 Unit(Vec2 a) {
  float l = Len(a);
  return l > .0001f ? a * (1 / l) : Vec2{1, 0};
}
Vec2 Clamp(Vec2 a, float r = .45f) {
  a = {std::clamp(a.x, -48.f + r, 48.f - r),
       std::clamp(a.y, -27.f + r, 27.f - r)};
  for (const auto &obstacle : GetArenaObstacles()) {
    Vec2 delta = a - obstacle.position;
    float distance = Len(delta), minimum = r + obstacle.radius;
    if (distance < minimum)
      a = obstacle.position + Unit(delta) * minimum;
  }
  return a;
}
float SegmentDistance(Vec2 p, Vec2 a, Vec2 b) {
  Vec2 d = b - a;
  float q = Len2(d);
  float t =
      q > 0 ? std::clamp(((p.x - a.x) * d.x + (p.y - a.y) * d.y) / q, 0.f, 1.f)
            : 0;
  return Len(p - (a + d * t));
}
void Down(float &v, float dt) { v = std::max(0.f, v - dt); }
int Index(SpellId s) { return static_cast<int>(s); }
} // namespace
float Simulation::Random() {
  rng_ ^= rng_ >> 12;
  rng_ ^= rng_ << 25;
  rng_ ^= rng_ >> 27;
  return float((rng_ * 2685821657736338717ULL) >> 40) / 16777216.f;
}
void Simulation::Start(std::uint64_t seed, HeroId hero,
                       const RunConfig &config) {
  const auto gen = state_.generation + 1;
  state_ = Snapshot{};
  state_.generation = gen;
  seed_ = seed;
  rng_ = seed ? seed : 1;
  nextId_ = 1;
  hero_ = hero;
  config_ = config;
  config_.fixedStep = std::clamp(config_.fixedStep, 1.f / 240, 1.f / 20);
  config_.maxEnemies = std::clamp<std::size_t>(config_.maxEnemies, 1, 512);
  config_.maxProjectiles =
      std::clamp<std::size_t>(config_.maxProjectiles, 1, 2048);
  config_.maxEffects = std::clamp<std::size_t>(config_.maxEffects, 1, 2048);
  config_.maxPickups = std::clamp<std::size_t>(config_.maxPickups, 1, 4096);
  for (auto &duration : config_.waveDurations)
    duration = std::max(.1f, duration);
  state_.enemies.reserve(config_.maxEnemies);
  state_.projectiles.reserve(config_.maxProjectiles);
  state_.effects.reserve(config_.maxEffects);
  state_.pickups.reserve(config_.maxPickups);
  accumulator_ = 0;
  dashLatched_ = dashQueued_ = false;
  spawnAccumulator_ = bossTimer_ = bossSummon_ = bossShield_ = wallTimer_ =
      chargeTimer_ = 0;
  spellCooldowns_.fill(0);
  casts_.fill(0);
  procCooldowns_.fill(0);
  speedBuff_ = powerBuff_ = shieldTime_ = bodyProcCooldown_ = critCooldown_ =
      resetTimer_ = 0;
  lowHpUsed_ = lowHpActive_ = false;
  eliteSpawned_ = 0;
  waveSpawned_.fill(0);
  beastRolled_ = false;
  damageDepth_ = 0;
  progression_.Reset(seed ^ 0x9e3779b97f4a7c15ULL, hero, config_.formResonance);
  state_.wave = 1;
  state_.waveCount = config_.sixWaves ? 6 : 4;
  state_.waveDuration = config_.waveDurations[0];
  state_.phase = RunPhase::Wave;
  Sync();
  state_.player.hp = state_.progression.build.stats.maxHp;
}
void Simulation::Restart() { Start(seed_, hero_, config_); }
void Simulation::Pause(bool p) {
  state_.paused = p;
  accumulator_ = 0;
}
void Simulation::Sync() {
  state_.spellCooldowns = spellCooldowns_;
  state_.progression = progression_.GetState();
  state_.player.hp =
      std::min(state_.player.hp, state_.progression.build.stats.maxHp);
}
void Simulation::RefreshProgression() { Sync(); }
bool Simulation::ChooseUpgrade(std::size_t i) {
  if (state_.paused ||
      (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss &&
       state_.phase != RunPhase::Rest))
    return false;
  bool ok = progression_.ChooseUpgrade(i);
  Sync();
  return ok;
}
bool Simulation::Reroll() {
  if (state_.paused ||
      (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss &&
       state_.phase != RunPhase::Rest))
    return false;
  bool ok = progression_.Reroll();
  Sync();
  return ok;
}
bool Simulation::BeginNextWave() {
  if (state_.phase != RunPhase::Rest)
    return false;
  progression_.EndRest();
  Sync();
  lowHpUsed_ = lowHpActive_ = false;
  eliteSpawned_ = 0;
  waveSpawned_.fill(0);
  spawnAccumulator_ = 0;
  state_.waveElapsed = 0;
  accumulator_ = 0;
  if (state_.wave >= state_.waveCount) {
    StartBoss();
    return true;
  }
  ++state_.wave;
  state_.waveDuration = config_.waveDurations[state_.wave - 1];
  state_.phase = RunPhase::Wave;
  return true;
}
void Simulation::Step(float dt, const InputState &input) {
  if (!std::isfinite(dt) || dt <= 0)
    return;
  if (state_.paused ||
      (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss)) {
    accumulator_ = 0;
    dashLatched_ = input.dash;
    dashQueued_ = false;
    return;
  }
  if (input.dash && !dashLatched_) {
    dashQueued_ = true;
  }
  dashLatched_ = input.dash;
  accumulator_ += std::min(dt, .25f);
  int budget = 64;
  while (accumulator_ + 1e-9 >= config_.fixedStep && budget-- > 0) {
    InputState stepInput = input;
    stepInput.dash = dashQueued_;
    dashQueued_ = false;
    Tick(config_.fixedStep, stepInput);
    accumulator_ -= config_.fixedStep;
    if (state_.phase != RunPhase::Wave && state_.phase != RunPhase::Boss) {
      accumulator_ = 0;
      break;
    }
  }
  Sync();
}
void Simulation::Tick(float dt, const InputState &input) {
  currentSource_ = SpellId::Count;
  ++state_.tick;
  state_.statistics.elapsed += dt;
  state_.waveElapsed += dt;
  auto &p = state_.player;
  const auto &stats = state_.progression.build.stats;
  Down(p.invulnerable, dt);
  Down(p.dashCooldown, dt);
  Down(speedBuff_, dt);
  Down(powerBuff_, dt);
  Down(shieldTime_, dt);
  Down(bodyProcCooldown_, dt);
  Down(critCooldown_, dt);
  for (auto &t : procCooldowns_)
    Down(t, dt);
  if (shieldTime_ == 0)
    p.shield = 0;
  Vec2 movement = input.move;
  if (!std::isfinite(movement.x) || !std::isfinite(movement.y))
    movement = {};
  if (Len2(movement) > 1)
    movement = Unit(movement);
  if (Len2(movement) > .001f)
    p.facing = Unit(movement);
  if (input.dash && p.dashCooldown <= 0) {
    p.dashRemaining = .3f;
    p.invulnerable = std::max(p.invulnerable, .3f);
    p.dashCooldown = std::max(2.f, stats.dashCooldown);
    if (progression_.HasTrigger(3))
      powerBuff_ = 2;
    EffectState e;
    e.kind = EffectKind::Dash;
    e.position = p.position;
    e.radius = .7f;
    e.life = .3f;
    AddEffect(e);
    if (progression_.HasMystery(1)) {
      e.kind = EffectKind::Burn;
      e.radius = 2;
      e.life = 3;
      e.damage = 4;
      e.interval = .5f;
      e.oneShot = false;
      e.element = Element::Fire;
      AddEffect(e);
    }
  }
  bool wasDash = p.dashRemaining > 0;
  if (wasDash) {
    float travel = std::min(dt, p.dashRemaining);
    p.position = Clamp(p.position + p.facing * (4.f / .3f * travel));
    Down(p.dashRemaining, dt);
    if (p.dashRemaining == 0 && progression_.HasMystery(2))
      Area(p.position, 2, 40, Element::Thunder, Form::None, false);
  } else
    p.position =
        Clamp(p.position +
              movement * (stats.moveSpeed * (speedBuff_ > 0 ? 1.15f : 1) * dt));
  p.hp = std::min(stats.maxHp, p.hp + stats.regen * dt);
  if (progression_.HasTrigger(7) && !lowHpUsed_ && p.hp < stats.maxHp * .3f) {
    lowHpActive_ = lowHpUsed_ = true;
  }
  if (p.hp >= stats.maxHp * .5f)
    lowHpActive_ = false;
  if (progression_.HasUnique(1)) {
    resetTimer_ += dt;
    if (resetTimer_ >= 20) {
      resetTimer_ -= 20;
      spellCooldowns_.fill(0);
    }
  }
  if (state_.phase == RunPhase::Wave) {
    SpawnWave(dt);
  }
  UpdateEnemies(dt);
  if (state_.phase == RunPhase::Dead)
    return;
  UpdateSpells(dt);
  UpdateProjectiles(dt);
  UpdateEffects(dt);
  ResolveDeaths();
  CollectPickups();
  if (state_.phase == RunPhase::Wave &&
      state_.waveElapsed >= state_.waveDuration)
    EndWave();
}
Vec2 Simulation::Edge() {
  float t = Random();
  if (Random() < .5f)
    return {Random() < .5f ? -47.f : 47.f, -26.f + t * 52};
  return {-47.f + t * 94, Random() < .5f ? -26.f : 26.f};
}
EnemyState *Simulation::Find(EntityId id) {
  for (auto &e : state_.enemies)
    if (e.id == id)
      return &e;
  return nullptr;
}
EnemyState *Simulation::Target(bool highest, float range) {
  EnemyState *best = nullptr;
  float d = range * range;
  for (auto &e : state_.enemies) {
    if (e.hp <= 0)
      continue;
    float q = Len2(e.position - state_.player.position);
    if (q > range * range)
      continue;
    if (!best ||
        (highest ? (e.hp > best->hp || (e.hp == best->hp && q < d)) : q < d)) {
      best = &e;
      d = q;
    }
  }
  return best;
}
Vec2 Simulation::DenseTarget(float radius) {
  Vec2 at = state_.player.position;
  int high = 0;
  for (const auto &e : state_.enemies) {
    int n = 0;
    for (const auto &other : state_.enemies)
      if (other.hp > 0 && Len2(e.position - other.position) < radius * radius)
        ++n;
    if (n > high) {
      high = n;
      at = e.position;
    }
  }
  return at;
}
EntityId Simulation::Spawn(EnemyRole role, Vec2 pos, EnemyTier tier,
                           EntityId owner, bool ignorePhaseCap) {
  static constexpr int capsA[] = {40, 70, 100, 130};
  static constexpr int capsB[] = {30, 50, 70, 90, 110, 130};
  std::size_t phaseCap = state_.phase == RunPhase::Boss ? 80
                         : config_.sixWaves
                             ? capsB[std::clamp(state_.wave - 1, 0, 5)]
                             : capsA[std::clamp(state_.wave - 1, 0, 3)];
  std::size_t cap = ignorePhaseCap ? config_.maxEnemies
                                   : std::min(config_.maxEnemies, phaseCap);
  if (state_.enemies.size() >= cap) {
    ++state_.statistics.droppedEvents;
    return 0;
  }
  EnemyState e;
  e.id = Id();
  e.role = role;
  e.tier = tier;
  e.position = Clamp(pos);
  e.owner = owner;
  const float hp[] = {18, 14, 90, 70, 120, 160, 14000, 80};
  const float speeds[] = {3.2f, 2.6f, 2.8f, 2, 1.6f, 1.2f, 1.5f, 6};
  const float damage[] = {6, 25, 30, 14, 0, 0, 35, 0};
  int i = int(role);
  int w = std::clamp(state_.wave - 1, 0, 5);
  const float hp4[] = {1, 1.5f, 2.2f, 3.2f};
  const float hp6[] = {1, 1.3f, 1.7f, 2.2f, 2.7f, 3.2f};
  float h = config_.sixWaves ? hp6[w] : hp4[std::min(w, 3)];
  const float damage4[] = {1, 1.15f, 1.3f, 1.5f};
  float d = config_.sixWaves ? 1 + .5f * float(w) / 5 : damage4[std::min(w, 3)];
  e.hp = e.maxHp = hp[i] * h;
  e.speed = speeds[i];
  e.damage = damage[i] * d;
  e.cooldown = (role == EnemyRole::Swarm ? 0 : 1);
  if (tier == EnemyTier::Elite) {
    e.hp *= 8;
    e.maxHp = e.hp;
    e.damage *= 1.5f;
    e.radius = .8f;
    e.mutations = 1 << int(Random() * 4);
    if (Random() < .5f)
      e.mutations |= 1 << int(Random() * 4);
    if (e.mutations & 1)
      e.speed *= 1.2f;
    if (e.mutations & 2)
      e.armor = 200;
  }
  if (role == EnemyRole::Boss) {
    e.hp = e.maxHp = config_.bossHp1;
    e.bossPhase = 1;
    e.radius = 1.8f;
  }
  state_.enemies.push_back(e);
  progression_.RecordEncounter(e.role, e.mutations);
  return e.id;
}
EntityId Simulation::TestSpawn(EnemyRole r, Vec2 p, EnemyTier t) {
  return Spawn(r, p, t, 0, true);
}
void Simulation::TestDamageEnemy(EntityId id, float d, Element element) {
  if (auto *e = Find(id))
    Hit(*e, d, element, Form::None, false);
  ResolveDeaths();
  Sync();
}
void Simulation::TestDamagePlayer(float damage) { Hurt(damage); }
void Simulation::SpawnWave(float dt) {
  (void)dt;
  if (!config_.spawning || state_.statistics.elapsed < config_.spawnGrace)
    return;
  int effective = config_.sixWaves ? std::min(3, (state_.wave - 1) * 4 / 6)
                                   : state_.wave - 1;
  int caps4[] = {40, 70, 100, 130};
  int caps6[] = {30, 50, 70, 90, 110, 130};
  int cap = config_.sixWaves ? caps6[state_.wave - 1] : caps4[effective];
  float f = state_.waveElapsed / state_.waveDuration;
  // Per-role budgets are distributed over six opening segments, not weighted
  // away.
  static constexpr int budgets[4][6] = {{240, 10, 0, 0, 0, 0},
                                        {300, 20, 12, 4, 0, 0},
                                        {420, 25, 20, 8, 6, 0},
                                        {520, 30, 25, 12, 8, 5}};
  static constexpr float starts[6] = {0, .15f, .3f, .45f, .6f, .75f};
  int remaining = 12;
  for (int role = 5; role >= 0 && remaining > 0; --role) {
    float begin = starts[role] * state_.waveDuration;
    if (state_.wave == 1)
      begin = std::max(begin, config_.spawnGrace);
    float progress = std::clamp((state_.waveElapsed - begin) /
                                    std::max(.1f, state_.waveDuration - begin),
                                0.f, 1.f);
    int desired = int(budgets[effective][role] * progress);
    while (waveSpawned_[role] < desired && int(state_.enemies.size()) < cap &&
           remaining-- > 0) {
      Spawn(EnemyRole(role), Edge(),
            role >= 2 ? EnemyTier::Variant : EnemyTier::Normal);
      ++waveSpawned_[role];
    }
  }
  int desired = effective == 2   ? (f > .65f ? 1 : 0)
                : effective == 3 ? (f > .8f   ? 2
                                    : f > .5f ? 1
                                              : 0)
                                 : 0;
  while (eliteSpawned_ < desired && int(state_.enemies.size()) < cap) {
    Spawn(eliteSpawned_ == 1 ? EnemyRole::Ranged : EnemyRole::Charger, Edge(),
          EnemyTier::Elite);
    ++eliteSpawned_;
  }
  if (!beastRolled_ && effective == 1 && f > .6f) {
    beastRolled_ = true;
    float luck =
        40 * (1 - std::exp(-std::max(0.f, state_.progression.build.stats.luck) /
                           30));
    if (Random() < .4f * (1 + luck * .015f))
      Spawn(EnemyRole::SpiritBeast, Edge(), EnemyTier::Variant);
  }
}
bool Simulation::AddEffect(EffectState e) {
  e.id = Id();
  e.duration = e.life;
  if (state_.effects.size() >= config_.maxEffects) {
    ++state_.statistics.droppedEvents;
    // Warnings may replace cosmetics or already-fired hazards, never another
    // pending warning whose owner could still execute its attack.
    if (e.hostile && e.warning > 0) {
      auto slot = std::find_if(state_.effects.begin(), state_.effects.end(),
                               [](const EffectState &old) {
                                 return !old.hostile || old.warning <= 0;
                               });
      if (slot != state_.effects.end()) {
        *slot = e;
        return true;
      }
    }
    return false;
  }
  state_.effects.push_back(e);
  return true;
}
void Simulation::AddProjectile(ProjectileState p) {
  if (state_.projectiles.size() >= config_.maxProjectiles) {
    ++state_.statistics.droppedEvents;
    return;
  }
  p.id = Id();
  p.previous = p.position;
  state_.projectiles.push_back(p);
  ++state_.statistics.shots;
}
void Simulation::Hurt(float d) {
  auto &p = state_.player;
  if (p.invulnerable > 0 || state_.phase == RunPhase::Dead ||
      state_.phase == RunPhase::Won) {
    ++state_.statistics.dodges;
    return;
  }
  const auto &s = state_.progression.build.stats;
  if (progression_.HasTrigger(2) && procCooldowns_[1] <= 0 && Random() < .08f) {
    p.shield += 20;
    shieldTime_ = 4;
    procCooldowns_[1] = 8;
  }
  d *= 1 - std::clamp(s.armor + (lowHpActive_ ? .15f : 0), 0.f, .6f);
  float shield = std::min(p.shield, d);
  p.shield -= shield;
  d -= shield;
  p.hp = std::max(0.f, p.hp - d);
  state_.statistics.damageTaken += d;
  for (auto spell : state_.progression.build.spells)
    if (spell.id == SpellId::FlameBody && spell.rank == 5 &&
        procCooldowns_[0] <= 0) {
      procCooldowns_[0] = 6;
      Area(p.position, 3, 40, Element::Fire, Form::Body, false);
      break;
    }
  if (p.hp <= 0)
    state_.phase = RunPhase::Dead;
}
float Simulation::ScaleDamage(float damage, Element element, Form form) const {
  const auto &b = state_.progression.build;
  const auto &s = b.stats;
  damage *= (s.power + (powerBuff_ > 0 ? .15f : 0));
  if (element != Element::None)
    damage *= 1 + s.elementBonus[int(element)];
  if (form != Form::None)
    damage *= 1 + s.formBonus[int(form)];
  if (form == Form::Sword && config_.formResonance && b.resonance.sword >= 4 &&
      state_.statistics.elapsed >= 8 &&
      std::fmod(state_.statistics.elapsed, 8.f) < 3)
    damage *= 1.3f;
  return damage;
}
void Simulation::Hit(EnemyState &e, float damage, Element element, Form form,
                     bool primary, float burn, float duration, float stun) {
  if (e.hp <= 0 || damageDepth_ >= 3)
    return;
  ++damageDepth_;
  const auto &b = state_.progression.build;
  const auto &s = b.stats;
  bool crit = false;
  if (primary) {
    damage = ScaleDamage(damage, element, form);
    crit = Random() < s.critChance;
    if (crit)
      damage *= s.critMultiplier;
  }
  if (e.stun > 0) {
    for (const auto &fx : state_.effects)
      if (fx.prisonVulnerability && fx.warning <= 0 &&
          Len2(e.position - fx.position) <= fx.radius * fx.radius) {
        damage *= 1.2f;
        break;
      }
  }
  if (config_.formResonance && b.resonance.array >= 4) {
    for (const auto &fx : state_.effects)
      if (!fx.hostile && fx.form == Form::Array && fx.warning <= 0 &&
          Len2(e.position - fx.position) <= fx.radius * fx.radius) {
        damage *= 1.2f;
        break;
      }
  }
  float originalHp = e.hp;
  if (e.armor > 0) {
    damage *= .7f;
    float a = std::min(e.armor, damage);
    e.armor -= a;
    damage -= a;
  }
  if (e.shield > 0) {
    float factor = element == Element::Fire ? 2.f : 1.f;
    float a = std::min(e.shield, damage * factor);
    e.shield -= a;
    damage -= a / factor;
  }
  e.hp -= std::max(0.f, damage);
  state_.statistics.damageDealt += std::min(originalHp, std::max(0.f, damage));
  ++state_.statistics.hits;
  if (burn > 0) {
    e.burnRemaining = std::max(e.burnRemaining, duration);
    e.burnStacks = std::min(b.resonance.fire >= 4 ? 3 : 1, e.burnStacks + 1);
    e.burnDps = std::max(
        e.burnDps, burn * (s.power + (powerBuff_ > 0 ? .15f : 0)) *
                       (1 + s.elementBonus[0]) *
                       (form == Form::None ? 1 : 1 + s.formBonus[int(form)]) *
                       s.burnMultiplier);
  }
  e.stun = std::max(e.stun, stun);
  if (element == Element::Thunder && e.role == EnemyRole::Exploder) {
    e.hp = 0;
    Area(e.position, 2.5f, 25, Element::None, Form::None, false);
  }
  if (primary && b.resonance.thunder >= 2 &&
      Random() < (b.resonance.thunder >= 4 ? .15f : .08f)) {
    Hit(e, 25, Element::None, Form::None, false, 0, 0,
        b.resonance.thunder >= 4 ? .5f : 0);
    EffectState fx;
    fx.kind = EffectKind::Lightning;
    fx.position = e.position;
    fx.radius = .7f;
    AddEffect(fx);
  }
  if (primary && form == Form::Body) {
    if (config_.formResonance && b.resonance.body >= 4 &&
        bodyProcCooldown_ <= 0 && Random() < .1f) {
      state_.player.invulnerable = std::max(state_.player.invulnerable, 1.f);
      bodyProcCooldown_ = 5;
    }
    if (progression_.HasMystery(5) && Random() < .12f)
      Area(e.position, 3, 30, Element::None, Form::None, false);
  }
  if (primary && crit) {
    if (progression_.HasTrigger(5) && procCooldowns_[4] <= 0 &&
        Random() < .1f) {
      procCooldowns_[4] = 1;
      Hit(e, 15, Element::None, Form::None, false);
    }
    if (progression_.HasMystery(6) && critCooldown_ <= 0) {
      critCooldown_ = .5f;
      for (int slot = 0; slot < int(spellCooldowns_.size()); ++slot)
        if (slot != Index(currentSource_))
          Down(spellCooldowns_[slot], .3f);
    }
  }
  --damageDepth_;
}
void Simulation::Area(Vec2 at, float radius, float damage, Element element,
                      Form form, bool primary, float burn, float duration,
                      float stun) {
  for (auto &e : state_.enemies)
    if (e.hp > 0 &&
        Len2(e.position - at) < (radius + e.radius) * (radius + e.radius))
      Hit(e, damage, element, form, primary, burn, duration, stun);
}
void Simulation::Chain(EntityId start, int count, float damage, float scale,
                       float radius, Form sourceForm, bool highest) {
  std::vector<EntityId> visited{start};
  auto *first = Find(start);
  if (!first)
    return;
  Vec2 at = first->position;
  for (int n = 0; n < count; ++n) {
    EnemyState *best = nullptr;
    float nearest = radius * radius;
    for (auto &e : state_.enemies) {
      float d = Len2(e.position - at);
      if (e.hp > 0 &&
          std::find(visited.begin(), visited.end(), e.id) == visited.end() &&
          d < radius * radius &&
          (!best || (highest ? e.hp > best->hp : d < nearest))) {
        nearest = d;
        best = &e;
      }
    }
    if (!best)
      break;
    EffectState fx;
    fx.kind = EffectKind::Line;
    fx.position = at;
    fx.end = best->position;
    fx.element = Element::Thunder;
    fx.life = .15f;
    AddEffect(fx);
    visited.push_back(best->id);
    at = best->position;
    Hit(*best,
        ScaleDamage(damage * scale, Element::Thunder, sourceForm) *
            state_.progression.build.stats.chainMultiplier,
        Element::Thunder, Form::None, false);
  }
}
void Simulation::UpdateEnemies(float dt) {
  const std::size_t initial = state_.enemies.size();
  for (std::size_t i = 0; i < initial; ++i) {
    auto &e = state_.enemies[i];
    if (e.hp <= 0)
      continue;
    if (e.burnRemaining > 0) {
      Down(e.burnRemaining, dt);
      Hit(e, e.burnDps * e.burnStacks * dt, Element::Fire, Form::None, false);
      if (e.burnRemaining == 0) {
        e.burnDps = 0;
        e.burnStacks = 0;
      }
    }
    if (e.hp <= 0)
      continue;
    Down(e.stun, dt);
    if (e.stun > 0)
      continue;
    Down(e.cooldown, dt);
    Vec2 delta = state_.player.position - e.position;
    float distance = Len(delta);
    Vec2 toward = Unit(delta);
    if (e.mutations & 8) {
      for (int orb = 0; orb < 3; ++orb) {
        float a = state_.statistics.elapsed * 2 + orb * 2 * Pi / 3;
        Vec2 at = e.position + Vec2{std::cos(a), std::sin(a)} * 1.7f;
        if (Len(at - state_.player.position) < .65f && e.cooldown <= 0) {
          Hurt(6);
          e.cooldown = .8f;
        }
      }
    }
    if (e.role == EnemyRole::Boss) {
      if (e.windup <= 0 && e.chargeRemaining <= 0)
        e.position = Clamp(e.position + toward * (e.speed * dt) *
                                            (distance > 5 ? 1.f : 0.f));
      bossTimer_ += dt;
      bossSummon_ += dt;
      bossShield_ += dt;
      if (e.bossPhase == 1) {
        if (bossTimer_ >= 6) {
          bossTimer_ -= 6;
          for (int k = 0; k < 3; ++k) {
            EffectState fx;
            fx.kind = EffectKind::Lightning;
            float a = k * 2 * Pi / 3;
            fx.position = state_.player.position +
                          Vec2{std::cos(a), std::sin(a)} * (k ? 2.5f : 0);
            fx.radius = 1.7f;
            fx.warning = 1;
            fx.life = .2f;
            fx.damage = 35;
            fx.hostile = true;
            AddEffect(fx);
          }
        }
        if (bossSummon_ >= 5) {
          bossSummon_ -= 5;
          for (int k = 0; k < 20 && state_.enemies.size() < 80; ++k)
            Spawn(EnemyRole::Swarm, Edge(), EnemyTier::Normal);
        }
        if (bossShield_ >= 15) {
          bossShield_ -= 15;
          for (auto &other : state_.enemies)
            if (other.id != e.id)
              other.shield = 40;
        }
      } else {
        wallTimer_ += dt;
        chargeTimer_ += dt;
        if (wallTimer_ >= 8) {
          wallTimer_ -= 8;
          EffectState fx;
          fx.kind = EffectKind::FireWall;
          fx.position = {0, 0};
          fx.radius = 48;
          fx.warning = 1.5f;
          fx.life = 6;
          fx.interval = .25f;
          fx.damage = 5;
          fx.oneShot = false;
          fx.hostile = true;
          AddEffect(fx);
        }
        if (chargeTimer_ >= 10 && e.windup <= 0 && e.chargeRemaining <= 0) {
          chargeTimer_ -= 10;
          e.windup = 1.2f;
          e.direction = toward;
          EffectState fx;
          fx.kind = EffectKind::Line;
          fx.position = e.position;
          fx.end = e.position + toward * 20;
          fx.owner = e.id;
          fx.radius = 1.8f;
          fx.warning = 1.2f;
          fx.life = .15f;
          fx.hostile = true;
          if (!AddEffect(fx)) {
            e.windup = 0;
            e.cooldown = .25f;
          }
        }
        if (bossSummon_ >= 8) {
          bossSummon_ = 0;
          for (int k = 0; k < 10 && state_.enemies.size() < 80; ++k)
            Spawn(k < 4 ? EnemyRole::Ranged : EnemyRole::Charger, Edge(),
                  EnemyTier::Variant);
        }
      }
    }
    if (e.windup > 0) {
      Down(e.windup, dt);
      if (e.windup == 0) {
        if (e.role == EnemyRole::Exploder) {
          if (distance < 2.5f + .4f)
            Hurt(e.damage);
          e.hp = 0;
          e.burnRemaining = 0;
        } else if (e.role == EnemyRole::Charger || e.role == EnemyRole::Boss)
          e.chargeRemaining = 1.4f;
        else if (e.role == EnemyRole::Ranged) {
          ProjectileState p;
          p.owner = e.id;
          p.position = e.position;
          p.velocity = Unit(e.target - e.position) * 7;
          p.damage = e.damage;
          p.hostile = true;
          p.life = Len(e.target - e.position) / 7;
          p.radius = .35f;
          AddProjectile(p);
        } else if (e.role == EnemyRole::Summoner) {
          int children = 0;
          for (const auto &o : state_.enemies)
            if (o.owner == e.id && o.hp > 0)
              ++children;
          for (int k = 0; k < std::min(3, 12 - children); ++k)
            Spawn(EnemyRole::Swarm,
                  e.position + Vec2{Random() * 2 - 1, Random() * 2 - 1},
                  EnemyTier::Normal, e.id);
        } else if (e.role == EnemyRole::Support) {
          int n = 0;
          for (auto &o : state_.enemies)
            if (o.id != e.id && o.hp > 0 &&
                Len2(o.position - e.position) < 64 && n++ < 3)
              o.shield = 40;
        }
      }
      continue;
    }
    if (e.chargeRemaining > 0) {
      Vec2 old = e.position;
      e.position = Clamp(e.position + e.direction * (9 * dt));
      Down(e.chargeRemaining, dt);
      if (SegmentDistance(state_.player.position, old, e.position) <
          e.radius + .4f) {
        Hurt(e.role == EnemyRole::Boss ? 45 : e.damage);
        e.chargeRemaining = 0;
      }
      continue;
    }
    if (e.role == EnemyRole::Boss)
      continue;
    if (e.role == EnemyRole::SpiritBeast) {
      e.position = Clamp(e.position - toward * e.speed * dt);
      continue;
    }
    if (e.role == EnemyRole::Swarm) {
      e.position = Clamp(e.position + toward * e.speed * dt);
      if (distance < e.radius + .4f && e.cooldown <= 0) {
        Hurt(e.damage);
        e.cooldown = .8f;
      }
    }
    if (e.role == EnemyRole::Exploder) {
      e.position = Clamp(e.position + toward * e.speed * dt);
      if (distance < 2.5f) {
        e.windup = .8f;
        EffectState fx;
        fx.owner = e.id;
        fx.kind = EffectKind::Explosion;
        fx.position = e.position;
        fx.radius = 2.5f;
        fx.warning = .8f;
        fx.life = .15f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
    if (e.role == EnemyRole::Charger) {
      e.position = Clamp(e.position + toward * e.speed * dt);
      if (distance < 12 && e.cooldown <= 0) {
        e.windup = 1;
        e.direction = toward;
        e.cooldown = (e.mutations & 1) ? 2.8f : 4;
        EffectState fx;
        fx.owner = e.id;
        fx.kind = EffectKind::Line;
        fx.position = e.position;
        fx.end = e.position + toward * 12.6f;
        fx.radius = e.radius;
        fx.warning = 1;
        fx.life = .15f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
    if (e.role == EnemyRole::Ranged) {
      if (distance > 8)
        e.position = Clamp(e.position + toward * e.speed * dt);
      else if (distance < 5)
        e.position = Clamp(e.position - toward * e.speed * dt);
      if (distance <= 9 && e.cooldown <= 0) {
        e.windup = .6f;
        e.target = state_.player.position;
        e.cooldown = (e.mutations & 1) ? 1.4f : 2;
        EffectState fx;
        fx.owner = e.id;
        fx.position = e.target;
        fx.radius = 1;
        fx.warning = .6f;
        fx.life = .15f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
    if (e.role == EnemyRole::Support || e.role == EnemyRole::Summoner) {
      if (distance > 7)
        e.position = Clamp(e.position + toward * e.speed * dt);
      if (e.cooldown <= 0) {
        e.windup = .6f;
        e.cooldown = e.role == EnemyRole::Support ? 5 : 4;
        EffectState fx;
        fx.owner = e.id;
        fx.kind = EffectKind::Shield;
        fx.position = e.position;
        fx.radius = e.role == EnemyRole::Support ? 8 : 2;
        fx.warning = .6f;
        fx.life = .2f;
        fx.hostile = true;
        if (!AddEffect(fx)) {
          e.windup = 0;
          e.cooldown = .25f;
        }
      }
    }
  }
}
void Simulation::UpdateSpells(float dt) {
  for (auto &cd : spellCooldowns_)
    Down(cd, dt);
  if (!config_.automaticSpells)
    return;
  for (const auto spell : state_.progression.build.spells)
    if (spellCooldowns_[Index(spell.id)] <= 0) {
      Cast(spell);
    }
}
void Simulation::Cast(SpellState spell) {
  currentSource_ = spell.id;
  auto c = GetSpellConfig(spell.id, spell.rank);
  const auto &b = state_.progression.build;
  const auto &stats = b.stats;
  float radius = c.radius * stats.area;
  spellCooldowns_[Index(spell.id)] =
      std::max(.08f, c.cooldown * ((spell.id == SpellId::FlameBody ||
                                    spell.id == SpellId::FlameRing)
                                       ? 1.f
                                       : 1 - stats.cooldownReduction));
  ++casts_[Index(spell.id)];
  Vec2 at = state_.player.position;
  auto field = [&](Vec2 pos, float r, float damage, float warning, float life,
                   bool oneShot, float interval) {
    EffectState fx;
    fx.kind =
        c.element == Element::Fire ? EffectKind::Burn : EffectKind::Lightning;
    fx.position = pos;
    fx.radius = r;
    fx.warning = warning;
    fx.life = life;
    fx.damage = damage;
    fx.interval = interval;
    fx.oneShot = oneShot;
    fx.element = c.element;
    fx.form = c.form;
    fx.source = spell.id;
    fx.prisonVulnerability = c.prisonVulnerability;
    fx.burnDps = c.burnDps;
    fx.burnDuration = c.burnDuration;
    fx.stun = c.stun;
    fx.followPlayer =
        c.followPlayer || (c.form == Form::Array && progression_.HasMystery(4));
    if (c.form == Form::Array && progression_.HasMystery(4))
      fx.radius *= .8f;
    AddEffect(fx);
  };
  if (spell.id == SpellId::FlamePalm || spell.id == SpellId::ThunderFist) {
    EntityId first = 0;
    for (auto &e : state_.enemies) {
      Vec2 delta = e.position - at;
      float length = Len(delta);
      float dot = length > 0 ? (delta.x * state_.player.facing.x +
                                delta.y * state_.player.facing.y) /
                                   length
                             : 1;
      if (e.hp > 0 && length < radius + e.radius &&
          dot >= std::cos(c.coneDegrees * .5f * Pi / 180)) {
        Hit(e, c.damage, c.element, c.form, true, c.burnDps, c.burnDuration);
        if (!first)
          first = e.id;
      }
    }
    if (!first) {
      // Automatic melee waits for a legal facing-cone target, like sword
      // targeting.
      spellCooldowns_[Index(spell.id)] = .15f;
      --casts_[Index(spell.id)];
      return;
    }
    if (first && c.chains)
      Chain(first, c.chains, c.damage, c.chainScale, c.chainRadius, c.form,
            c.highestHpChain);
    EffectState fx;
    fx.kind = EffectKind::Slash;
    fx.position = at;
    fx.end = at + state_.player.facing * radius;
    fx.radius = radius;
    fx.element = c.element;
    fx.form = c.form;
    fx.source = spell.id;
    fx.life = .2f;
    AddEffect(fx);
    if (c.fireAfterwave && first) {
      EffectState after;
      after.kind = EffectKind::Burn;
      after.position = at;
      after.radius = radius;
      after.damage = 2;
      after.life = 2;
      after.interval = .5f;
      after.oneShot = false;
      after.element = Element::Fire;
      after.form = Form::Body;
      after.source = spell.id;
      AddEffect(after);
    }
    return;
  }
  if (spell.id == SpellId::FlameBody || spell.id == SpellId::FusionPalm) {
    if (spell.id == SpellId::FusionPalm && !Target(false, radius + .8f)) {
      spellCooldowns_[Index(spell.id)] = .15f;
      --casts_[Index(spell.id)];
      return;
    }
    Area(at, radius, c.damage, c.element, c.form, true, c.burnDps,
         c.burnDuration, c.stun);
    EffectState fx;
    fx.kind = spell.id == SpellId::FusionPalm ? EffectKind::Explosion
                                              : EffectKind::Burn;
    fx.position = at;
    fx.radius = radius;
    fx.life = .2f;
    fx.element = c.element;
    AddEffect(fx);
    return;
  }
  if (spell.id == SpellId::FlameRing) {
    if (c.expandingRing && state_.statistics.elapsed >= 6 &&
        std::fmod(state_.statistics.elapsed, 6.f) < 1.5f)
      radius = 5 * stats.area;
    int count =
        c.count + (config_.formResonance && b.resonance.sword >= 2 ? 1 : 0);
    for (int k = 0; k < count; ++k) {
      float angle = state_.statistics.elapsed * 3 * c.rotationMultiplier +
                    k * 2 * Pi / count;
      Vec2 pos = at + Vec2{std::cos(angle), std::sin(angle)} * radius;
      Area(pos, 1.2f, c.damage, c.element, c.form);
      EffectState fx;
      fx.kind = EffectKind::Slash;
      fx.position = pos;
      fx.radius = .6f;
      fx.life = .5f;
      fx.element = c.element;
      fx.form = c.form;
      fx.source = spell.id;
      AddEffect(fx);
    }
    return;
  }
  if (spell.id == SpellId::FlameSword || spell.id == SpellId::ThunderSword ||
      spell.id == SpellId::FusionSwords) {
    auto *target = Target(spell.id != SpellId::ThunderSword);
    if (!target) {
      spellCooldowns_[Index(spell.id)] = .15f;
      --casts_[Index(spell.id)];
      return;
    }
    int count = c.count;
    int extra =
        progression_.HasMystery(3) && casts_[Index(spell.id)] % 5 == 0 ? 2 : 0;
    for (int k = 0; k < count + extra; ++k) {
      EnemyState *aim = target;
      if (k > 0 && c.count > 1) {
        EnemyState *alternative = nullptr;
        float near = 1e9f;
        for (auto &e : state_.enemies)
          if (e.hp > 0 && e.id != target->id && Len2(e.position - at) < near) {
            near = Len2(e.position - at);
            alternative = &e;
          }
        if (alternative)
          aim = alternative;
      }
      float spread = (k - (count - 1) * .5f) * .12f;
      Vec2 dir = Unit(aim->position - at);
      dir = {dir.x * std::cos(spread) - dir.y * std::sin(spread),
             dir.x * std::sin(spread) + dir.y * std::cos(spread)};
      ProjectileState p;
      p.position = at;
      p.source = spell.id;
      p.pathBurn = spell.id == SpellId::FlameSword;
      if (spell.id == SpellId::FusionSwords) {
        p.orbitRemaining = .3f;
        p.orbitAngle = k * Pi;
      }
      p.velocity = dir * 14;
      p.speed = 14;
      p.turnMultiplier = c.turnMultiplier;
      p.target = aim->id;
      p.damage = c.damage * (k >= count ? .6f : 1);
      p.life = c.radius / 14;
      p.pierce =
          c.pierce + (config_.formResonance && b.resonance.sword >= 2 ? 1 : 0);
      p.element = c.element;
      p.form = c.form;
      p.homing = spell.id != SpellId::FlameSword;
      p.returning = c.returning;
      p.chains = c.chains;
      p.chainScale = c.chainScale;
      p.burnDps = c.burnDps;
      p.burnDuration = c.burnDuration;
      AddProjectile(p);
    }
    return;
  }
  if (spell.id == SpellId::SwordRain) {
    Vec2 center = DenseTarget(radius);
    int count =
        c.count + (config_.formResonance && b.resonance.sword >= 2 ? 1 : 0);
    int extra =
        progression_.HasMystery(3) && casts_[Index(spell.id)] % 5 == 0 ? 2 : 0;
    for (int k = 0; k < count + extra; ++k) {
      float angle = Random() * Pi * 2, r = std::sqrt(Random()) * radius;
      Vec2 pos = center + Vec2{std::cos(angle), std::sin(angle)} * r;
      field(pos, 1.2f, c.damage * (k >= count ? .6f : 1.f), c.warning, .12f,
            true, .5f);
    }
    if (c.electricAfterfield)
      field(center, radius, 2.5f, c.warning, 2, false, .5f);
    return;
  }
  if (c.form == Form::Array) {
    Vec2 center = spell.id == SpellId::FlameArray ? at : DenseTarget(radius);
    float duration =
        c.duration *
        (config_.formResonance && b.resonance.array >= 2 ? 1.25f : 1);
    if (spell.id == SpellId::HeavenlyThunder) {
      field(center, radius, c.damage, c.warning, .2f, true, .5f);
      for (int k = 0; k < c.afterBolts; ++k)
        field(center, radius, c.damage * c.afterBoltScale,
              c.warning + .4f * (k + 1), .15f, true, .5f);
    } else
      field(center, radius, c.damage, 0, duration, false, c.interval);
  }
}
void Simulation::UpdateProjectiles(float dt) {
  for (auto &p : state_.projectiles) {
    if (p.life <= 0)
      continue;
    currentSource_ = p.source;
    p.previous = p.position;
    if (p.orbitRemaining > 0) {
      Down(p.orbitRemaining, dt);
      p.orbitAngle += dt * 16;
      p.position = state_.player.position +
                   Vec2{std::cos(p.orbitAngle), std::sin(p.orbitAngle)} * 1.5f;
      continue;
    }
    if (p.homing) {
      auto *target = Find(p.target);
      if (target && target->hp > 0) {
        Vec2 desired = Unit(target->position - p.position) * p.speed;
        float turn = std::min(1.f, dt * 8 * p.turnMultiplier);
        p.velocity = p.velocity * (1 - turn) + desired * turn;
        p.velocity = Unit(p.velocity) * p.speed;
      }
    }
    p.position = p.position + p.velocity * dt;
    p.life -= dt;
    if (p.pathBurn) {
      p.pathTick -= dt;
      if (p.pathTick <= 0) {
        p.pathTick = .15f;
        EffectState trail;
        trail.position = p.position;
        trail.kind = EffectKind::Burn;
        trail.radius = .5f;
        trail.life = 2;
        trail.interval = .5f;
        trail.damage = 1;
        trail.oneShot = false;
        trail.element = Element::Fire;
        trail.form = Form::Sword;
        trail.source = p.source;
        AddEffect(trail);
      }
    }
    if (p.hostile) {
      bool hit = SegmentDistance(state_.player.position, p.previous,
                                 p.position) < p.radius + .4f;
      if (hit) {
        Hurt(p.damage);
        p.life = 0;
      }
      if (p.life <= 0) {
        EffectState fx;
        fx.kind = EffectKind::Burn;
        fx.position = p.position;
        fx.radius = 1;
        fx.life = 4;
        fx.damage = 1.5f;
        fx.interval = .5f;
        fx.oneShot = false;
        fx.hostile = true;
        AddEffect(fx);
      }
    } else {
      for (auto &e : state_.enemies) {
        if (p.pierce <= 0 || e.hp <= 0 ||
            std::find(p.hit.begin(), p.hit.end(), e.id) != p.hit.end())
          continue;
        if (SegmentDistance(e.position, p.previous, p.position) <
            p.radius + e.radius) {
          p.hit.push_back(e.id);
          Hit(e, p.damage, p.element, p.form, true, p.burnDps, p.burnDuration);
          if (p.chains)
            Chain(e.id, p.chains, p.damage, p.chainScale, 3, p.form);
          if (--p.pierce <= 0) {
            if (!p.returning || p.returned)
              p.life = 0;
            break;
          }
        }
      }
      if (p.life <= 0 && p.returning && !p.returned) {
        p.returned = true;
        p.hit.clear();
        p.pierce = 6;
        p.velocity = Unit(state_.player.position - p.position) * p.speed;
        p.life = Len(state_.player.position - p.position) / p.speed + .1f;
      }
    }
    if (std::abs(p.position.x) > 50 || std::abs(p.position.y) > 29)
      p.life = 0;
  }
  state_.projectiles.erase(
      std::remove_if(state_.projectiles.begin(), state_.projectiles.end(),
                     [](const auto &p) { return p.life <= 0; }),
      state_.projectiles.end());
}
void Simulation::UpdateEffects(float dt) {
  const std::size_t n = state_.effects.size();
  for (std::size_t i = 0; i < n; ++i) {
    auto &e = state_.effects[i];
    currentSource_ = e.source;
    if (e.followPlayer)
      e.position = state_.player.position;
    if (e.owner && e.hostile) {
      auto *owner = Find(e.owner);
      if (!owner || owner->hp <= 0) {
        e.life = 0;
        continue;
      }
      if (owner->stun > 0 && e.warning > 0)
        continue;
    }
    if (e.warning > 0) {
      Down(e.warning, dt);
      continue;
    }
    e.life -= dt;
    e.tick -= dt;
    if (e.damage <= 0 || e.tick > 0)
      continue;
    e.tick = std::max(.05f, e.interval);
    if (e.hostile) {
      bool inside = false;
      if (e.kind == EffectKind::FireWall) {
        float elapsed = e.duration - e.life;
        float x = 48 - elapsed * 5, y = 27 - elapsed * 2.5f;
        inside = std::abs(state_.player.position.x) > x - 1 ||
                 std::abs(state_.player.position.y) > y - 1;
        e.radius = x;
      } else
        inside = Len2(state_.player.position - e.position) <
                 (e.radius + .4f) * (e.radius + .4f);
      if (inside)
        Hurt(e.damage);
    } else
      Area(e.position, e.radius, e.damage, e.element, e.form, true, e.burnDps,
           e.burnDuration, e.stun);
    if (e.oneShot)
      e.damage = 0;
  }
  state_.effects.erase(
      std::remove_if(state_.effects.begin(), state_.effects.end(),
                     [](const auto &e) { return e.life <= 0; }),
      state_.effects.end());
}
void Simulation::ResolveDeaths() {
  // Remove before death procs so secondary explosions can never re-kill an
  // entity.
  bool bossDefeated = false;
  for (;;) {
    std::vector<EnemyState> dead;
    for (const auto &e : state_.enemies)
      if (e.hp <= 0)
        dead.push_back(e);
    if (dead.empty())
      break;
    state_.enemies.erase(
        std::remove_if(state_.enemies.begin(), state_.enemies.end(),
                       [](const auto &e) { return e.hp <= 0; }),
        state_.enemies.end());
    for (auto &e : dead) {
      if (e.role == EnemyRole::Boss) {
        if (state_.phase == RunPhase::Dead)
          continue;
        if (e.bossPhase == 1) {
          BossPhaseTwo(e);
          state_.enemies.push_back(e);
        } else {
          bossDefeated = true;
        }
        continue;
      }
      ++state_.statistics.kills;
      if (progression_.HasTrigger(1))
        speedBuff_ = 2;
      if (progression_.HasTrigger(4) && state_.statistics.kills % 25 == 0)
        state_.player.hp = std::min(
            state_.progression.build.stats.maxHp,
            state_.player.hp + state_.progression.build.stats.maxHp * .05f);
      float xp[] = {1, 1, 4, 4, 6, 8, 0, 4};
      PickupState pickup;
      pickup.id = Id();
      pickup.position = e.position;
      pickup.xp = e.tier == EnemyTier::Elite ? 25 : xp[int(e.role)];
      auto relic =
          progression_.OnKill(e.tier, e.role == EnemyRole::SpiritBeast);
      if (relic) {
        pickup.hasRelic = true;
        pickup.relic = *relic;
      }
      if (state_.pickups.size() < config_.maxPickups)
        state_.pickups.push_back(pickup);
      else {
        progression_.AddXp(pickup.xp);
        if (pickup.hasRelic)
          progression_.GrantRelic(pickup.relic);
        ++state_.statistics.droppedEvents;
      }
      if (e.burnRemaining > 0) {
        if (state_.progression.build.resonance.fire >= 4)
          Area(e.position, 2, 20, Element::None, Form::None, false);
        if (progression_.HasTrigger(6) && Random() < .1f)
          Area(e.position, 1.5f, 12, Element::None, Form::None, false);
      }
      if (e.mutations & 4)
        for (int k = 0; k < 2; ++k) {
          EntityId id = Spawn(e.role, e.position + Vec2{float(k * 2 - 1), 0},
                              EnemyTier::Variant);
          if (auto *child = Find(id)) {
            child->hp = child->maxHp = e.maxHp * .4f;
            child->mutations = 0;
          }
        }
    }
  }
  if (bossDefeated) {
    state_.phase = RunPhase::Won;
    state_.bossPhase = 2;
    state_.projectiles.clear();
    state_.effects.clear();
    state_.enemies.clear();
    CollectPickups(true);
    progression_.BeginRest();
    progression_.GenerateBossRewards();
  }
  Sync();
}
void Simulation::CollectPickups(bool all) {
  float radius = state_.progression.build.stats.pickupRadius;
  for (auto &p : state_.pickups)
    if (all || Len2(p.position - state_.player.position) <= radius * radius) {
      progression_.AddXp(p.xp);
      if (p.hasRelic)
        progression_.GrantRelic(p.relic);
      p.id = 0;
    }
  state_.pickups.erase(std::remove_if(state_.pickups.begin(),
                                      state_.pickups.end(),
                                      [](const auto &p) { return p.id == 0; }),
                       state_.pickups.end());
  Sync();
}
void Simulation::EndWave() {
  CollectPickups(true);
  state_.enemies.clear();
  state_.projectiles.clear();
  state_.effects.clear();
  state_.phase = RunPhase::Rest;
  progression_.OnWaveEnded();
  progression_.BeginRest();
  Sync();
}
void Simulation::StartBoss() {
  state_.phase = RunPhase::Boss;
  state_.bossPhase = 1;
  state_.waveElapsed = 0;
  bossTimer_ = bossSummon_ = bossShield_ = wallTimer_ = chargeTimer_ = 0;
  Spawn(EnemyRole::Boss, {0, -12}, EnemyTier::Boss);
  for (int k = 0; k < 2; ++k)
    Spawn(EnemyRole::Support, {float(k * 12 - 6), -16}, EnemyTier::Variant);
}
void Simulation::BossPhaseTwo(EnemyState &e) {
  e.hp = e.maxHp = config_.bossHp2;
  e.bossPhase = 2;
  e.windup = e.chargeRemaining = 0;
  state_.bossPhase = 2;
  bossTimer_ = bossSummon_ = bossShield_ = wallTimer_ = chargeTimer_ = 0;
  state_.projectiles.clear();
  state_.effects.clear();
  EffectState fx;
  fx.kind = EffectKind::Explosion;
  fx.position = e.position;
  fx.radius = 6;
  fx.warning = 1.2f;
  fx.life = .4f;
  AddEffect(fx);
  for (int k = 0; k < 10 && state_.enemies.size() + 1 <
                                std::min<std::size_t>(80, config_.maxEnemies);
       ++k)
    Spawn(k < 4 ? EnemyRole::Ranged : EnemyRole::Charger, Edge(),
          EnemyTier::Variant);
}
} // namespace dywx::mvp
