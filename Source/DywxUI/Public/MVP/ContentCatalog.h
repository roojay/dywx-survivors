#pragma once
#include "MVP/Types.h"
#include <array>
#include <string_view>
namespace dywx::mvp {
// Rank-dependent authoritative numeric spell table, in grid units and seconds.
struct SpellConfig {
  SpellId id{};
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
};
SpellConfig GetSpellConfig(SpellId id, int rank = 1);
const std::array<PassiveConfig, 5> &PassiveCatalog();
const std::array<RelicConfig, 15> &RelicCatalog();
std::string_view RarityName(Rarity rarity);
std::string_view AffixName(const Affix &affix);
std::string DescribeAffix(const Affix &affix);
std::string DescribeSpell(SpellId spell, int rank);
float EffectiveLuck(float luck);
int RefineValue(Rarity rarity);
} // namespace dywx::mvp
