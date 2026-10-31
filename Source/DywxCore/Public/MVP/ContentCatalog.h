#pragma once
#include "MVP/Export.h"
#include "MVP/Types.h"
#include <array>
#include <string_view>
namespace dywx::mvp {
enum class SpellBehavior { Legacy, Cone, Projectile, Field, Companion, Talisman };
// Rank-dependent authoritative numeric spell table, in grid units and seconds.
struct DYWXCORE_API SpellConfig {
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
struct DYWXCORE_API PassiveConfig {
  PassiveId id;
  std::string_view name, description;
};
struct DYWXCORE_API RelicConfig {
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
DYWXCORE_API SpellConfig GetSpellConfig(SpellId id, int rank = 1);
DYWXCORE_API const std::array<PassiveConfig, kPassiveCount> &PassiveCatalog();
DYWXCORE_API const std::array<RelicConfig, kRelicCount> &RelicCatalog();
DYWXCORE_API std::string_view RarityName(Rarity rarity);
DYWXCORE_API std::string_view AffixName(const Affix &affix);
DYWXCORE_API std::string DescribeAffix(const Affix &affix);
DYWXCORE_API std::string DescribeSpell(SpellId spell, int rank);
DYWXCORE_API bool IsFusion(SpellId id);
struct DYWXCORE_API FusionRecipe { HeroId hero; SpellId first, second, result; PassiveId passive = PassiveId::Count; };
DYWXCORE_API const std::array<FusionRecipe, kFusionCount>& FusionCatalog();
DYWXCORE_API int ElementResonanceCode(Element element);
DYWXCORE_API int FormResonanceCode(Form form);
DYWXCORE_API AffixKind AspectAffix(int code);
DYWXCORE_API std::string DescribeRelic(int catalogId);
DYWXCORE_API std::string DescribeResonance(int code);
DYWXCORE_API std::string DescribeFusion(const FusionRecipe& recipe);
DYWXCORE_API std::array<float,2> AffixRange(AffixKind kind,Rarity rarity);
DYWXCORE_API std::vector<int> RealmRelicPool(int realm);
DYWXCORE_API float EffectiveLuck(float luck);
DYWXCORE_API int RefineValue(Rarity rarity);
} // namespace dywx::mvp
