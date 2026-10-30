#pragma once
#include "MVP/Types.h"
#include <array>
#include <string_view>
namespace dywx::mvp {
enum class CaveId { ThunderMarsh, EmberMountain, MistForest };
enum class AnomalyId { Lightning, SpiritTide, Fissure, Fog, Eclipse, Array, Count };
struct CaveDefinition {
  CaveId id; std::string_view name, bossName, palette;
  std::array<std::string_view, 6> enemies;
  std::array<float, 6> roleWeights;
  std::array<int, 4> relicPool;
  std::array<std::string_view, 3> opportunities;
};
const std::array<CaveDefinition, 3>& CaveCatalog();
std::string_view AnomalyName(AnomalyId id);
std::string_view AnomalyDescription(AnomalyId id);
// All new balance values are provisional and configurable. Dimensions match paired v0.1 config.
struct ArenaConfig {
  float halfWidth=48, halfHeight=27, cornerCut=5, centralClearRadius=10;
  float gridSize=1, navigationClearance=.6f, peripheralMargin=4;
  int obstacleCount=24, placementAttempts=256, veinCount=6;
  float minObstacleRadius=1, maxObstacleRadius=2.5f, maxObstacleFraction=.25f;
};
struct ArenaLayout {
  std::uint64_t seed=0; CaveId cave=CaveId::ThunderMarsh; ArenaConfig config;
  std::vector<ArenaObstacle> obstacles;
  std::vector<Vec2> veins, hazards, beastRoute, edgeSpawns;
  bool usedFallback=false;
  bool Contains(Vec2 point, float radius=0) const;
  bool Walkable(Vec2 point, float radius=.45f) const;
  Vec2 Constrain(Vec2 point, float radius=.45f) const;
  float ObstacleFraction() const;
};
ArenaLayout GenerateArena(std::uint64_t seed, CaveId cave=CaveId::ThunderMarsh,
                          const ArenaConfig& config={});
bool ValidateArena(const ArenaLayout& layout);
struct AnomalyConfig {
  float interval=24, activeDuration=16, warning=1.2f, pulseInterval=4;
  float hazardRadius=2.2f, lightningDamage=22, fissureDamage=16;
  float thunderReward=1.3f, thunderRewardDuration=5;
  float tideSpawn=1.35f, tideCurrency=1.5f, fissureChargers=1.8f;
  float fogVision=14, fogRanged=1.8f, fogBeasts=2;
  float eclipseEnemyPower=1.3f, eclipseXp=1.35f, arrayPower=1.25f, arrayRadius=3;
};
std::string DescribeAnomaly(AnomalyId id, const AnomalyConfig& config);
struct AnomalyModifiers {
  float spawn=1, currency=1, chargers=1, ranged=1, beasts=1;
  float nightEnemyPower=1, xp=1, visionRange=1000, playerPower=1, thunderPower=1;
};
struct ArenaHazardEvent {
  AnomalyId anomaly=AnomalyId::Lightning; Vec2 position;
  float radius=0, damage=0; bool warning=false, absorbed=false;
};
struct AnomalyState {
  bool enabled=false, active=false; AnomalyId id=AnomalyId::Lightning;
  Vec2 anchor; float elapsed=0, remaining=0, warningRemaining=0;
  std::array<bool, 6> encountered{};
};
class AnomalySystem {
public:
  void Reset(std::uint64_t seed, const ArenaLayout& layout, bool layerTwo,
             const AnomalyConfig& config={});
  // dt advances combat time only. Callers must not tick during pause/rest.
  std::vector<ArenaHazardEvent> Step(float dt, Vec2 player, bool hasThunder);
  AnomalyModifiers Modifiers(Vec2 player) const;
  const AnomalyState& State() const { return state_; }
  const AnomalyConfig& Config() const { return config_; }
private:
  friend class SaveCodec; // Versioned checkpoint preserves anomaly schedule without reroll.
  ArenaLayout layout_; AnomalyConfig config_; AnomalyState state_;
  std::array<AnomalyId,6> order_{}; std::size_t cycle_=0;
  double clock_=0, nextStart_=0, nextPulse_=0, strikeAt_=-1, endAt_=0;
  double thunderUntil_=0;
};
} // namespace dywx::mvp
