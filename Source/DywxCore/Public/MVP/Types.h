#pragma once
#include "MVP/Export.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace dywx::mvp {
using EntityId = std::uint64_t;
struct DYWXCORE_API Vec2 {
  float x = 0, y = 0;
  Vec2 operator+(Vec2 b) const { return {x + b.x, y + b.y}; }
  Vec2 operator-(Vec2 b) const { return {x - b.x, y - b.y}; }
  Vec2 operator*(float s) const { return {x * s, y * s}; }
};
enum class HeroId { Body, Sword, Beast, Count };
enum class SpellId {
  FlamePalm,
  ThunderFist,
  FlameBody,
  FlameSword,
  FlameRing,
  ThunderSword,
  SwordRain,
  FlameArray,
  ThunderPrison,
  HeavenlyThunder,
  FusionPalm,
  FusionSwords,
  MetalPalm, WoodPalm, WaterPalm, EarthPalm,
  MetalSword, WoodSword, WaterSword, EarthSword,
  MetalArray, WoodArray, WaterArray, EarthArray,
  FireBeast, ThunderBeast, WoodBeast, WaterBeast,
  FireTalisman, ThunderTalisman, MetalTalisman, EarthTalisman,
  FusionBeast,
  AscendantPalm, AscendantSword, AscendantBeast,
  Count
};
enum class PassiveId {
  BodyScripture,
  SwordScripture,
  FireScripture,
  ThunderScripture,
  ClearMind,
  BeastScripture, MetalScripture, WoodScripture, WaterScripture, EarthScripture, TalismanScripture,
  Count
};
enum class Element { Fire, Thunder, None, Metal, Wood, Water, Earth, Count };
enum class Form { Body, Sword, Array, None, Beast, Talisman, Count };
inline constexpr std::size_t kSpellCount = static_cast<std::size_t>(SpellId::Count);
inline constexpr std::size_t kPassiveCount = static_cast<std::size_t>(PassiveId::Count);
inline constexpr std::size_t kRelicCount = 80, kFusionCount = 6, kResonanceCount = 11;
enum class Rarity { Common, Spirit, Immortal, Dao };
enum class EnemyRole {
  Swarm,
  Exploder,
  Charger,
  Ranged,
  Support,
  Summoner,
  Boss,
  SpiritBeast
};
enum class EnemyTier { Normal, Variant, Elite, Boss };
enum class RunPhase { Title, Wave, Rest, Boss, Won, Dead };
enum class EffectKind {
  Circle,
  Line,
  Burn,
  Lightning,
  Slash,
  Shield,
  FireWall,
  Dash,
  Explosion
};
struct DYWXCORE_API Stats {
  float maxHp = 100, armor = 0, regen = .5f, power = 1, cooldownReduction = 0,
        area = 1, critChance = .05f, critMultiplier = 1.5f, moveSpeed = 5,
        pickupRadius = 2, dashCooldown = 6, luck = 0;
  float xpMultiplier = 1, burnMultiplier = 1, chainMultiplier = 1;
  std::array<bool, 7> triggers{};
  std::array<bool, 6> mysteries{};
  std::array<bool, 2> uniques{};
  std::array<float, static_cast<std::size_t>(Element::Count)> elementBonus{};
  std::array<float, static_cast<std::size_t>(Form::Count)> formBonus{};
};
struct DYWXCORE_API SpellState {
  SpellId id = SpellId::FlamePalm;
  int rank = 1;
};
struct DYWXCORE_API Resonance {
  int fire = 0, thunder = 0, body = 0, sword = 0, array = 0;
  int metal = 0, wood = 0, water = 0, earth = 0, beast = 0, talisman = 0;
};
struct DYWXCORE_API BuildState {
  HeroId hero = HeroId::Body;
  std::vector<SpellState> spells;
  std::vector<PassiveId> passives;
  Stats stats;
  Resonance resonance;
};
enum class AffixKind {
  MaxHp,
  Armor,
  Regen,
  Power,
  Cooldown,
  Area,
  Crit,
  MoveSpeed,
  Pickup,
  Dash,
  Luck,
  Fire,
  Thunder,
  Body,
  Sword,
  Array,
  Resonance,
  Trigger,
  Mystery,
  Unique,
  Metal, Wood, Water, Earth, Beast, Talisman, Count
};
struct DYWXCORE_API Affix {
  AffixKind kind = AffixKind::Power;
  float value = 0;
  int code = 0;
};
struct DYWXCORE_API Relic {
  EntityId id = 0;
  int catalogId = 0;
  Rarity rarity = Rarity::Common;
  std::vector<Affix> affixes;
  bool locked = false;
  bool carryOut = false;
};
enum class UpgradeKind { NewSpell, ImproveSpell, Passive, Fusion };
struct DYWXCORE_API UpgradeOffer {
  UpgradeKind kind = UpgradeKind::NewSpell;
  SpellId spell = SpellId::FlamePalm;
  PassiveId passive = PassiveId::ClearMind;
};
struct DYWXCORE_API ProgressionState {
  BuildState build;
  float xp = 0;
  int level = 1, pendingUpgrades = 0, rerollsRemaining = 2;
  float xpRequired = 23;
  std::vector<UpgradeOffer> offers;
  std::vector<Relic> equipped, bag;
  int pendingRefineXp = 0, pityWaves = 0;
  int systemLayer = 1, sealsRemaining = 3, skippedUpgrades = 0, skipSpiritStones = 0;
  std::array<bool, kSpellCount> sealedSpells{};
  std::array<bool, kPassiveCount> sealedPassives{};
  std::vector<Relic> bossRewards;
  std::array<bool, kSpellCount> seenSpells{};
  std::array<bool, kRelicCount> seenRelics{};
  std::array<bool, kFusionCount> seenFusions{};
  std::array<bool, kPassiveCount> seenPassives{};
  std::array<bool, 8> seenEnemies{};
  std::array<bool, 4> seenMutations{};
  std::array<bool, kResonanceCount> seenResonances{};
};
struct DYWXCORE_API PlayerState {
  Vec2 position{}, facing{1, 0};
  float hp = 100, shield = 0, invulnerable = 0, dashRemaining = 0,
        dashCooldown = 0;
};
struct DYWXCORE_API EnemyState {
  EntityId id = 0;
  EnemyRole role = EnemyRole::Swarm;
  EnemyTier tier = EnemyTier::Normal;
  Vec2 position{}, direction{1, 0}, target{};
  float hp = 18, maxHp = 18, shield = 0, armor = 0, radius = .45f, speed = 3.2f,
        damage = 6, cooldown = 0, windup = 0, chargeRemaining = 0, stun = 0,
        burnRemaining = 0, burnDps = 0;
  float slowRemaining = 0, slowFraction = 0;
  int burnStacks = 0, mutations = 0, bossPhase = 0;
  EntityId owner = 0;
};
struct DYWXCORE_API ProjectileState {
  EntityId id = 0, owner = 0, target = 0;
  Vec2 position{}, previous{}, velocity{};
  float radius = .2f, damage = 0, life = 2, speed = 14, turnMultiplier = 1,
        pathTick = 0, orbitRemaining = 0, orbitAngle = 0, burnDps = 0,
        burnDuration = 0, chainScale = .5f;
  int pierce = 1, chains = 0;
  Element element = Element::None;
  Form form = Form::None;
  SpellId source = SpellId::Count;
  bool hostile = false, homing = false, returning = false, returned = false,
       pathBurn = false;
  std::vector<EntityId> hit;
};
struct DYWXCORE_API EffectState {
  EntityId id = 0, owner = 0;
  EffectKind kind = EffectKind::Circle;
  Vec2 position{}, end{};
  float radius = 1, warning = 0, life = .3f, duration = .3f, tick = 0,
        interval = .5f, damage = 0, burnDps = 0, burnDuration = 0, stun = 0;
  Element element = Element::None;
  Form form = Form::None;
  SpellId source = SpellId::Count;
  bool hostile = false, followPlayer = false, oneShot = true,
       prisonVulnerability = false;
  int pulses = 0;
};
struct DYWXCORE_API PickupState {
  EntityId id = 0;
  Vec2 position{};
  float xp = 0;
  bool hasRelic = false;
  Relic relic;
  int spiritStones = 0;
};
struct DYWXCORE_API RunStatistics {
  std::uint64_t kills = 0, shots = 0, hits = 0, dodges = 0;
  float damageDealt = 0, damageTaken = 0, elapsed = 0;
  std::uint64_t droppedEvents = 0;
};
struct DYWXCORE_API RunConfig {
  bool sixWaves = false, formResonance = true;
  std::array<float, 6> waveDurations{100, 120, 140, 160, 90, 95};
  float fixedStep = 1.f / 60.f, spawnGrace = 5;
  std::size_t maxEnemies = 160, maxProjectiles = 512, maxEffects = 512,
              maxPickups = 1024;
  // Harness-only overrides; production defaults above and boss values below
  // match v0.1.
  float bossHp1 = 14000, bossHp2 = 16000;
  bool spawning = true, automaticSpells = true;
  static RunConfig SixWave() {
    RunConfig c;
    c.sixWaves = true;
    c.waveDurations = {70, 75, 80, 85, 90, 95};
    return c;
  }
};
struct DYWXCORE_API InputState {
  Vec2 move{};
  bool dash = false;
};
struct DYWXCORE_API Snapshot {
  std::uint64_t generation = 0, tick = 0;
  RunPhase phase = RunPhase::Title;
  bool paused = false;
  int wave = 0, waveCount = 4, bossPhase = 0;
  float waveElapsed = 0, waveDuration = 100;
  PlayerState player;
  std::array<float, kSpellCount> spellCooldowns{};
  ProgressionState progression;
  std::vector<EnemyState> enemies;
  std::vector<ProjectileState> projectiles;
  std::vector<EffectState> effects;
  std::vector<PickupState> pickups;
  RunStatistics statistics;
};
struct DYWXCORE_API ArenaObstacle {
  Vec2 position;
  float radius;
};
inline std::array<ArenaObstacle, 6> GetArenaObstacles() {
  return {{{{-18, -9}, 1.6f},
           {{15, 11}, 1.8f},
           {{-30, 15}, 2.f},
           {{30, -15}, 2.f},
           {{-7, 19}, 1.5f},
           {{7, -19}, 1.5f}}};
}
} // namespace dywx::mvp
