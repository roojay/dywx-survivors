#pragma once
#include "MVP/Progression.h"
#include "MVP/Types.h"
namespace dywx::mvp {
class Simulation {
public:
  void Start(std::uint64_t seed, HeroId hero,
             const RunConfig &config = RunConfig{});
  void Step(float dt, const InputState &input = {});
  bool ChooseUpgrade(std::size_t index);
  bool Reroll();
  bool BeginNextWave();
  void Pause(bool paused);
  void Restart();
  const Snapshot &GetSnapshot() const { return state_; }
  const RunConfig &GetConfig() const { return config_; }
  Progression &GetProgression() { return progression_; }
  // inventory commands only; call RefreshProgression afterwards
  const Progression &GetProgression() const { return progression_; }
  void RefreshProgression();
  // Deterministic harness hooks; never bound to shipping input.
  EntityId TestSpawn(EnemyRole role, Vec2 position,
                     EnemyTier tier = EnemyTier::Normal);
  void TestDamageEnemy(EntityId id, float damage,
                       Element element = Element::None);
  void TestDamagePlayer(float damage);
  void TestCast(SpellId id, int rank = 1) { Cast({id, rank}); }
  void TestSetPlayer(Vec2 position, float hp) {
    state_.player.position = position;
    state_.player.hp = hp;
  }

private:
  Snapshot state_;
  Progression progression_;
  RunConfig config_;
  std::uint64_t seed_ = 1, rng_ = 1, nextId_ = 1;
  HeroId hero_ = HeroId::Body;
  double accumulator_ = 0;
  bool dashLatched_ = false, dashQueued_ = false;
  float spawnAccumulator_ = 0, bossTimer_ = 0, bossSummon_ = 0, bossShield_ = 0,
        wallTimer_ = 0, chargeTimer_ = 0;
  std::array<float, 12> spellCooldowns_{};
  std::array<int, 12> casts_{};
  std::array<float, 7> procCooldowns_{};
  float speedBuff_ = 0, powerBuff_ = 0, shieldTime_ = 0, bodyProcCooldown_ = 0,
        critCooldown_ = 0, resetTimer_ = 0;
  bool lowHpUsed_ = false, lowHpActive_ = false;
  std::array<int, 6> waveSpawned_{};
  int eliteSpawned_ = 0;
  bool beastRolled_ = false;
  int damageDepth_ = 0;
  SpellId currentSource_ = SpellId::Count;
  float Random();
  EntityId Id() { return nextId_++; }
  void Tick(float dt, const InputState &input);
  void SpawnWave(float dt);
  void UpdateEnemies(float dt);
  void UpdateSpells(float dt);
  void UpdateProjectiles(float dt);
  void UpdateEffects(float dt);
  void CollectPickups(bool all = false);
  void ResolveDeaths();
  EntityId Spawn(EnemyRole role, Vec2 position, EnemyTier tier,
                 EntityId owner = 0, bool ignorePhaseCap = false);
  Vec2 Edge();
  EnemyState *Find(EntityId id);
  EnemyState *Target(bool highest = false, float range = 100);
  Vec2 DenseTarget(float radius);
  float ScaleDamage(float damage, Element element, Form form) const;
  void Hit(EnemyState &enemy, float damage, Element element, Form form,
           bool primary = true, float burn = 0, float burnDuration = 0,
           float stun = 0);
  void Area(Vec2 at, float radius, float damage, Element element, Form form,
            bool primary = true, float burn = 0, float burnDuration = 0,
            float stun = 0);
  void Chain(EntityId start, int count, float damage, float scale, float radius,
             Form sourceForm, bool highest = false);
  void Hurt(float damage);
  bool AddEffect(EffectState effect);
  void AddProjectile(ProjectileState projectile);
  void Cast(SpellState spell);
  void EndWave();
  void StartBoss();
  void BossPhaseTwo(EnemyState &boss);
  void Sync();
};
} // namespace dywx::mvp
