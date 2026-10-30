#pragma once
#include "MVP/Types.h"
#include <array>
#include <string_view>
namespace dywx::mvp {
enum class SpellBehavior { Legacy, Cone, Projectile, Field, Companion, Talisman };
// Rank-dependent authoritative numeric spell table, in grid units and seconds.
struct SpellConfig {
  SpellId id{};
  SpellBehavior behavior = SpellBehavior::Legacy;
  bool provisional = false;
  float slowFraction = 0, slowDuration = 0, healOnHit = 0, shieldOnCast = 0;
  std::string_view name;
  Element element = Element::None;
  Form form = Form::None;
  float damage = 0, cooldown = 1, radius = 0, duration = 0, interval = 0,
        warning = 0;
  float burnDps = 0, burnDuration = 0, stun = 0, chainScale = 0,
        chainRadius = 3, coneDegrees = 360;
  int count = 1, pierce = 1, chains = 0;
  float turnMultiplier = 1, rotationMultiplier = 1;
  bool followPlayer = false, returning = false, highestHpChain = false;
  bool fireAfterwave = false, onHitExplosion = false, expandingRing = false,
       electricAfterfield = false, prisonVulnerability = false;
  int afterBolts = 0;
  float afterBoltScale = .4f;
};
struct PassiveConfig {
  PassiveId id;
  std::string_view name, description;
};
struct RelicConfig {
  int id;
  std::string_view name;
  Rarity rarity;
  AffixKind primary = AffixKind::Power, aspect = AffixKind::Fire;
  int trigger = 1, mystery = 1;
  bool provisional = false;
  AffixKind secondary = AffixKind::Armor;
  int resonanceCode = 0, uniqueCode = 1;
  int realm = -1; // -1 shared legacy pool; 0 ThunderMarsh, 1 EmberMountain, 2 MistForest.
};
SpellConfig GetSpellConfig(SpellId id, int rank = 1);
const std::array<PassiveConfig, kPassiveCount> &PassiveCatalog();
const std::array<RelicConfig, kRelicCount> &RelicCatalog();
std::string_view RarityName(Rarity rarity);
std::string_view AffixName(const Affix &affix);
std::string DescribeAffix(const Affix &affix);
std::string DescribeSpell(SpellId spell, int rank);
bool IsFusion(SpellId id);
struct FusionRecipe { HeroId hero; SpellId first, second, result; PassiveId passive = PassiveId::Count; };
const std::array<FusionRecipe, kFusionCount>& FusionCatalog();
int ElementResonanceCode(Element element);
int FormResonanceCode(Form form);
AffixKind AspectAffix(int code);
std::string DescribeRelic(int catalogId);
std::string DescribeResonance(int code);
std::string DescribeFusion(const FusionRecipe& recipe);
std::array<float,2> AffixRange(AffixKind kind,Rarity rarity);
std::vector<int> RealmRelicPool(int realm);
float EffectiveLuck(float luck);
int RefineValue(Rarity rarity);
} // namespace dywx::mvp
