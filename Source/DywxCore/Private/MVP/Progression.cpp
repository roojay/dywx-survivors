#include "MVP/Progression.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>
#include <sstream>
#include <iomanip>
namespace dywx::mvp {
namespace {
bool Has(const Relic &r, AffixKind k, int code) {
  return std::any_of(r.affixes.begin(), r.affixes.end(), [&](const Affix &a) {
    return a.kind == k && a.code == code;
  });
}
int &Count(Resonance &r, int code) {
  switch (code) {
  case 0:
    return r.fire;
  case 1:
    return r.thunder;
  case 2:
    return r.body;
  case 3:
    return r.sword;
  case 4: return r.array;
  case 5: return r.metal;
  case 6: return r.wood;
  case 7: return r.water;
  case 8: return r.earth;
  case 9: return r.beast;
  default: return r.talisman;
  }
}
bool Same(const UpgradeOffer &a, const UpgradeOffer &b) {
  return a.kind == b.kind &&
         (a.kind == UpgradeKind::Passive ? a.passive == b.passive
                                         : a.spell == b.spell);
}
} // namespace
float Progression::Unit() {
  return static_cast<float>((rng_() >> 8) * (1.0 / 16777216.0));
}
int Progression::Index(int n) {
  return n > 0 ? std::min(n - 1, int(Unit() * n)) : 0;
}
void Progression::Reset(std::uint32_t seed, HeroId hero, bool forms) {
  dropRiskWeight_=1;preferredRelics_.clear();lastBossOrdinal_=0;
  fullGame_ = false;spellPoolUnlocked_=relicPoolUnlocked_=true;
  permanentBase_ = Stats{};
  rng_.seed(seed);
  state_ = {};
  state_.build.hero = hero;
  formResonance_ = forms;
  resting_ = false;
  bossRewardsGenerated_ = false;
  pityWaves_ = 0;
  immortalThisWave_ = false;
  pendingRefineXp_ = 0;
  state_.pendingRefineXp = 0;
  undo_.clear();
  consumed_.fill(false);
  nextRelic_ = 1;
  state_.build.spells.push_back(
      {hero == HeroId::Body ? SpellId::FlamePalm : hero == HeroId::Beast ? SpellId::WoodBeast : SpellId::FlameSword, 1});
  state_.build.passives.push_back(hero == HeroId::Body
                                      ? PassiveId::BodyScripture
                                      : hero == HeroId::Beast ? PassiveId::BeastScripture : PassiveId::SwordScripture);
  state_.seenSpells[int(state_.build.spells.front().id)] = true;
  Recompute();
}
void Progression::ConfigureFullGame(int layer, const Stats& bonuses, int extraRerolls) {
  fullGame_=true;permanentBase_=ClampStats(bonuses);SetSystemLayer(layer);
  state_.rerollsRemaining+=std::max(0,extraRerolls);Recompute();
  if(state_.pendingUpgrades>0)MakeOffers();
}
void Progression::SetContentUnlocks(bool spells,bool relics){spellPoolUnlocked_=spells;relicPoolUnlocked_=relics;if(state_.pendingUpgrades>0)MakeOffers();}
void Progression::SetSystemLayer(int layer){state_.systemLayer=std::clamp(layer,1,3);if(fullGame_)formResonance_=state_.systemLayer>=2;Recompute();}
bool Progression::SealOffer(std::size_t index){
  if(state_.systemLayer<2 || state_.sealsRemaining<=0 || index>=state_.offers.size())return false;
  const auto o=state_.offers[index];
  if(o.kind==UpgradeKind::Passive)state_.sealedPassives[int(o.passive)]=true;
  else state_.sealedSpells[int(o.spell)]=true;
  --state_.sealsRemaining;MakeOffers();return true;
}
bool Progression::SkipUpgrade(){
  if(state_.systemLayer<2 || state_.pendingUpgrades<=0 || state_.offers.empty())return false;
  --state_.pendingUpgrades;++state_.skippedUpgrades;state_.skipSpiritStones+=5;
  state_.offers.clear();AddRawXp(4);if(state_.pendingUpgrades>0)MakeOffers();return true;
}
int Progression::TakeSkipStones(){int n=state_.skipSpiritStones;state_.skipSpiritStones=0;return n;}
void Progression::ImportAccountProgress(const ProgressionState &known) {
  SetPityWaves(known.pityWaves);
  immortalThisWave_ = false;
  MergeDiscoveries(known);
}
void Progression::MergeDiscoveries(const ProgressionState &previous) {
  for (std::size_t i = 0; i < state_.seenSpells.size(); ++i)
    state_.seenSpells[i] = state_.seenSpells[i] || previous.seenSpells[i];
  for (std::size_t i = 0; i < state_.seenRelics.size(); ++i)
    state_.seenRelics[i] = state_.seenRelics[i] || previous.seenRelics[i];
  for (std::size_t i = 0; i < state_.seenFusions.size(); ++i)
    state_.seenFusions[i] = state_.seenFusions[i] || previous.seenFusions[i];
  for (std::size_t i = 0; i < state_.seenPassives.size(); ++i)
    state_.seenPassives[i] = state_.seenPassives[i] || previous.seenPassives[i];
  for (std::size_t i = 0; i < state_.seenEnemies.size(); ++i)
    state_.seenEnemies[i] = state_.seenEnemies[i] || previous.seenEnemies[i];
  for (std::size_t i = 0; i < state_.seenMutations.size(); ++i)
    state_.seenMutations[i] =
        state_.seenMutations[i] || previous.seenMutations[i];
  for (std::size_t i = 0; i < state_.seenResonances.size(); ++i)
    state_.seenResonances[i] =
        state_.seenResonances[i] || previous.seenResonances[i];
}
void Progression::RecordEncounter(EnemyRole role, int mutations) {
  const auto index = static_cast<std::size_t>(role);
  if (index >= state_.seenEnemies.size())
    return;
  state_.seenEnemies[index] = true;
  for (std::size_t i = 0; i < state_.seenMutations.size(); ++i)
    if (mutations & (1 << i))
      state_.seenMutations[i] = true;
}
bool Progression::HasSpell(SpellId id) const {
  return std::any_of(state_.build.spells.begin(), state_.build.spells.end(),
                     [&](auto s) { return s.id == id; });
}
bool Progression::HasPassive(PassiveId id) const {
  return std::find(state_.build.passives.begin(), state_.build.passives.end(),
                   id) != state_.build.passives.end();
}
void Progression::AddXp(float xp) {
  AddRawXp(xp * state_.build.stats.xpMultiplier);
}
void Progression::AddRawXp(float xp) {
  if (!std::isfinite(xp) || xp <= 0)
    return;
  state_.xp += xp;
  while (state_.xp >= state_.xpRequired) {
    state_.xp -= state_.xpRequired;
    ++state_.level;
    ++state_.pendingUpgrades;
    state_.xpRequired = 15.f + 8.f * state_.level;
  }
  if (state_.offers.empty() && state_.pendingUpgrades > 0)
    MakeOffers();
}
std::vector<UpgradeOffer> Progression::LegalOffers() const {
  std::vector<UpgradeOffer> out;
  for (int i = 0; i < (fullGame_ ? int(kSpellCount) : 10); ++i) {
    auto id = SpellId(i);
    if(IsFusion(id) || state_.sealedSpells[i])continue;
    if(!spellPoolUnlocked_ && i>=12 && !(state_.build.hero==HeroId::Beast && (id==SpellId::WoodBeast || id==SpellId::WaterBeast)))continue;
    if ((id == SpellId::FlamePalm && state_.build.hero != HeroId::Body) ||
        (id == SpellId::FlameSword && state_.build.hero != HeroId::Sword) ||
        (id == SpellId::WoodBeast && state_.build.hero != HeroId::Beast))
      continue;
    auto it =
        std::find_if(state_.build.spells.begin(), state_.build.spells.end(),
                     [&](auto s) { return s.id == id; });
    if (it != state_.build.spells.end()) {
      if (it->rank < 5)
        out.push_back({UpgradeKind::ImproveSpell, id, {}});
    } else if (state_.build.spells.size() < 6 && !consumed_[i])
      out.push_back({UpgradeKind::NewSpell, id, {}});
  }
  if (state_.build.passives.size() < 4)
    for (int i = 2; i < (fullGame_ && spellPoolUnlocked_ ? int(kPassiveCount) : 5); ++i)
      if (i != int(PassiveId::BeastScripture) && !state_.sealedPassives[i] && !HasPassive(PassiveId(i)))
        out.push_back({UpgradeKind::Passive, {}, PassiveId(i)});
  return out;
}
bool Progression::Affinity(const UpgradeOffer &o) const {
  auto r=state_.build.resonance;
  if(o.kind==UpgradeKind::Passive){
    int code=o.passive==PassiveId::FireScripture?0:o.passive==PassiveId::ThunderScripture?1:o.passive==PassiveId::ClearMind?4:o.passive==PassiveId::MetalScripture?5:o.passive==PassiveId::WoodScripture?6:o.passive==PassiveId::WaterScripture?7:o.passive==PassiveId::EarthScripture?8:10;
    return Count(r,code)>0;
  }
  const auto c=GetSpellConfig(o.spell);int e=ElementResonanceCode(c.element),f=FormResonanceCode(c.form);
  return (e>=0 && Count(r,e)>0)||(f>=0 && Count(r,f)>0);
}
void Progression::MakeOffers() {
  state_.offers.clear();
  auto pool = LegalOffers();
  if (pool.empty()) {
    state_.pendingUpgrades = 0;
    return;
  }
  const float luck = 1 + .015f * EffectiveLuck(state_.build.stats.luck);
  auto pick = [&](bool affinity) {
    std::array<int, 3> counts{};
    for (auto &o : pool)
      if (!affinity || Affinity(o))
        ++counts[int(o.kind)];
    std::array<float, 3> groups{30 * luck, 45.f, 25 * luck};
    if (state_.build.spells.size() >= 6) {
      groups[1] += groups[0];
      groups[0] = 0;
    }
    std::vector<float> w;
    float total = 0;
    for (auto &o : pool) {
      float x = (!affinity || Affinity(o)) && counts[int(o.kind)]
                    ? groups[int(o.kind)] / counts[int(o.kind)]
                    : 0;
      auto rank5 = [&](SpellId id) {
        return std::any_of(state_.build.spells.begin(),
                           state_.build.spells.end(),
                           [&](auto s) { return s.id == id && s.rank == 5; });
      };
      for(const auto& recipe:FusionCatalog())
        if(recipe.hero==state_.build.hero){
          if(recipe.passive!=PassiveId::Count){
            if((o.kind==UpgradeKind::Passive && o.passive==recipe.passive && rank5(recipe.first)) ||
               (o.kind!=UpgradeKind::Passive && o.spell==recipe.first && HasPassive(recipe.passive)))x*=2;
          } else if(o.kind!=UpgradeKind::Passive && ((o.spell==recipe.first && rank5(recipe.second)) || (o.spell==recipe.second && rank5(recipe.first))))x*=2;
        }
      w.push_back(x);
      total += x;
    }
    if (total <= 0)
      return false;
    float roll = Unit() * total;
    std::size_t i = 0;
    for (; i + 1 < w.size() && roll >= w[i]; ++i)
      roll -= w[i];
    state_.offers.push_back(pool[i]);
    pool.erase(pool.begin() + i);
    return true;
  };
  pick(true);
  while (state_.offers.size() < 3 && !pool.empty())
    if (!pick(false))
      break;
}
bool Progression::ChooseUpgrade(std::size_t i) {
  if (i >= state_.offers.size() || state_.pendingUpgrades <= 0)
    return false;
  auto o = state_.offers[i];
  auto legal = LegalOffers();
  if (std::none_of(legal.begin(), legal.end(),
                   [&](auto x) { return Same(x, o); }))
    return false;
  if (o.kind == UpgradeKind::NewSpell) {
    state_.build.spells.push_back({o.spell, 1});
    state_.seenSpells[int(o.spell)] = true;
  } else if (o.kind == UpgradeKind::ImproveSpell) {
    for (auto &s : state_.build.spells)
      if (s.id == o.spell)
        ++s.rank;
  } else if (o.kind == UpgradeKind::Passive)
    state_.build.passives.push_back(o.passive);
  --state_.pendingUpgrades;
  state_.offers.clear();
  Recompute();
  if (state_.pendingUpgrades > 0)
    MakeOffers();
  return true;
}
bool Progression::Reroll() {
  if (state_.rerollsRemaining <= 0 || state_.offers.empty())
    return false;
  --state_.rerollsRemaining;
  MakeOffers();
  return true;
}
bool Progression::CanFuse(SpellId result) const {
  if(HasSpell(result))return false;
  auto max=[&](SpellId id){return std::any_of(state_.build.spells.begin(),state_.build.spells.end(),[&](auto s){return s.id==id && s.rank==5;});};
  for(const auto& r:FusionCatalog())if(r.result==result && r.hero==state_.build.hero)return max(r.first)&&(r.passive!=PassiveId::Count?HasPassive(r.passive):max(r.second));
  return false;
}
bool Progression::Fuse(SpellId result){
  if(!CanFuse(result)||(!resting_&&state_.offers.empty()))return false;
  for(std::size_t i=0;i<FusionCatalog().size();++i){const auto&r=FusionCatalog()[i];if(r.result!=result)continue;
    auto&v=state_.build.spells;v.erase(std::remove_if(v.begin(),v.end(),[&](auto sp){return sp.id==r.first||(r.passive==PassiveId::Count && sp.id==r.second);}),v.end());
    v.push_back({result,1});consumed_[int(r.first)]=true;if(r.passive==PassiveId::Count)consumed_[int(r.second)]=true;
    state_.seenSpells[int(result)]=true;state_.seenFusions[i]=true;Recompute();if(!state_.offers.empty())MakeOffers();return true;
  }return false;
}
Stats Progression::ClampStats(Stats s) {
  s.maxHp = std::max(1.f, s.maxHp);
  s.armor = std::clamp(s.armor, 0.f, .6f);
  s.regen = std::max(0.f, s.regen);
  s.power = std::max(0.f, s.power);
  s.cooldownReduction = std::clamp(s.cooldownReduction, 0.f, .5f);
  s.area = std::clamp(s.area, 0.f, 2.f);
  s.critChance = std::clamp(s.critChance, 0.f, .75f);
  s.moveSpeed = std::clamp(s.moveSpeed, 0.f, 8.f);
  s.pickupRadius = std::clamp(s.pickupRadius, 0.f, 8.f);
  s.dashCooldown = std::max(2.f, s.dashCooldown);
  s.luck = std::max(0.f, s.luck);
  return s;
}
BuildState Progression::Derive(const std::vector<Relic> &equipment) const {
  BuildState b = state_.build;
  b.stats = permanentBase_;
  b.resonance = {};
  auto &s = b.stats;
  auto &r = b.resonance;
  auto addAspects=[&](SpellId id){auto c=GetSpellConfig(id);int e=ElementResonanceCode(c.element),f=FormResonanceCode(c.form);if(e>=0)++Count(r,e);if(f>=0)++Count(r,f);};
  for(auto sp:b.spells){
    if(IsFusion(sp.id)){for(const auto&recipe:FusionCatalog())if(recipe.result==sp.id){addAspects(recipe.first);if(recipe.passive==PassiveId::Count)addAspects(recipe.second);}}
    else addAspects(sp.id);
  }
  for (auto p : b.passives)
    switch (p) {
    case PassiveId::BodyScripture:
      s.maxHp += 20;
      s.armor += .08f;
      s.dashCooldown -= .5f;
      ++r.body;
      break;
    case PassiveId::SwordScripture:
      s.cooldownReduction += .1f;
      s.power += .08f;
      s.xpMultiplier += .1f;
      ++r.sword;
      break;
    case PassiveId::FireScripture:
      s.power += .12f;
      s.burnMultiplier += .15f;
      ++r.fire;
      break;
    case PassiveId::ThunderScripture:
      s.critChance += .08f;
      s.chainMultiplier += .15f;
      ++r.thunder;
      break;
    case PassiveId::BeastScripture:s.regen+=.3f;s.pickupRadius+=1;s.formBonus[int(Form::Beast)]+=.1f;++r.beast;break;
    case PassiveId::MetalScripture:s.critChance+=.06f;++r.metal;break;
    case PassiveId::WoodScripture:s.regen+=.4f;++r.wood;break;
    case PassiveId::WaterScripture:s.cooldownReduction+=.08f;++r.water;break;
    case PassiveId::EarthScripture:s.armor+=.06f;s.maxHp+=15;++r.earth;break;
    case PassiveId::TalismanScripture:s.area+=.12f;++r.talisman;break;
    case PassiveId::ClearMind:
      s.xpMultiplier += .1f;
      s.luck += 5;
      ++r.array;
      break;
    default:
      break;
    }
  bool resonanceAdded = false;
  for (auto &relic : equipment)
    for (auto &a : relic.affixes) {
      switch (a.kind) {
      case AffixKind::MaxHp:
        s.maxHp += a.value;
        break;
      case AffixKind::Armor:
        s.armor += a.value;
        break;
      case AffixKind::Regen:
        s.regen += a.value;
        break;
      case AffixKind::Power:
        s.power += a.value;
        break;
      case AffixKind::Cooldown:
        s.cooldownReduction += a.value;
        break;
      case AffixKind::Area:
        s.area += a.value;
        break;
      case AffixKind::Crit:
        s.critChance += a.value;
        break;
      case AffixKind::MoveSpeed:
        s.moveSpeed += 5 * a.value;
        break;
      case AffixKind::Pickup:
        s.pickupRadius += a.value;
        break;
      case AffixKind::Dash:
        s.dashCooldown -= a.value;
        break;
      case AffixKind::Luck:
        s.luck += a.value;
        break;
      case AffixKind::Fire:
      case AffixKind::Thunder:
        s.elementBonus[int(a.kind) - int(AffixKind::Fire)] += a.value;
        break;
      case AffixKind::Body:
      case AffixKind::Sword:
      case AffixKind::Array:
        s.formBonus[int(a.kind) - int(AffixKind::Body)] += a.value;
        break;
      case AffixKind::Metal:s.elementBonus[int(Element::Metal)]+=a.value;break;
      case AffixKind::Wood:s.elementBonus[int(Element::Wood)]+=a.value;break;
      case AffixKind::Water:s.elementBonus[int(Element::Water)]+=a.value;break;
      case AffixKind::Earth:s.elementBonus[int(Element::Earth)]+=a.value;break;
      case AffixKind::Beast:s.formBonus[int(Form::Beast)]+=a.value;break;
      case AffixKind::Talisman:s.formBonus[int(Form::Talisman)]+=a.value;break;
      case AffixKind::Count:break;
      case AffixKind::Resonance:
        if (!resonanceAdded && a.code >= 0 && a.code < int(kResonanceCount)) {
          ++Count(r, a.code);
          resonanceAdded = true;
        }
        break;
      case AffixKind::Trigger:
        if (a.code >= 1 && a.code <= 7)
          s.triggers[a.code - 1] = true;
        break;
      case AffixKind::Mystery:
        if (a.code >= 1 && a.code <= 6)
          s.mysteries[a.code - 1] = true;
        break;
      case AffixKind::Unique:
        if (a.code >= 1 && a.code <= 2)
          s.uniques[a.code - 1] = true;
        break;
      }
    }
  if (s.uniques[1]) {
    ++r.fire;
    ++r.thunder;
  }
  if (r.fire >= 2)
    s.burnMultiplier += .25f;
  if (formResonance_ && r.body >= 2) {
    s.armor += .1f;
    s.formBonus[0] += .15f;
  }
  // Provisional full-game resonance rewards. Combat-specific legacy rewards remain in Simulation.
  if(r.metal>=2)s.elementBonus[int(Element::Metal)]+=.15f;
  if(r.metal>=4)s.critChance+=.1f;
  if(r.wood>=2)s.regen+=.5f;
  if(r.wood>=4){s.regen+=.5f;s.elementBonus[int(Element::Wood)]+=.2f;}
  if(r.water>=2)s.elementBonus[int(Element::Water)]+=.15f;
  if(r.water>=4)s.cooldownReduction+=.1f;
  if(r.earth>=2)s.armor+=.08f;
  if(r.earth>=4){s.maxHp+=30;s.elementBonus[int(Element::Earth)]+=.2f;}
  if(formResonance_ && r.beast>=2)s.formBonus[int(Form::Beast)]+=.15f;
  if(formResonance_ && r.beast>=4){s.regen+=.8f;s.formBonus[int(Form::Beast)]+=.15f;}
  if(formResonance_ && r.talisman>=2)s.formBonus[int(Form::Talisman)]+=.15f;
  if(formResonance_ && r.talisman>=4){s.area+=.2f;s.formBonus[int(Form::Talisman)]+=.15f;}
  b.stats = ClampStats(s);
  return b;
}
void Progression::Recompute() {
  state_.build = Derive(state_.equipped);
  for (auto passive : state_.build.passives)
    state_.seenPassives[static_cast<std::size_t>(passive)] = true;
  const auto &r = state_.build.resonance;
  const std::array<int, kResonanceCount> counts{r.fire, r.thunder, r.body, r.sword, r.array,r.metal,r.wood,r.water,r.earth,r.beast,r.talisman};
  for (std::size_t i = 0; i < counts.size(); ++i)
    if (counts[i] >= 2 && (i < 2 || (i>=5 && i<=8) || formResonance_))
      state_.seenResonances[i] = true;
}
bool Progression::HasTrigger(int c) const {
  return c >= 1 && c <= 7 && state_.build.stats.triggers[c - 1];
}
bool Progression::HasMystery(int c) const {
  return c >= 1 && c <= 6 && state_.build.stats.mysteries[c - 1];
}
bool Progression::HasUnique(int c) const {
  return c >= 1 && c <= 2 && state_.build.stats.uniques[c - 1];
}
bool Progression::Compatible(const Relic &r,
                             std::optional<std::size_t> replace) const {
  if (Has(r, AffixKind::Mystery, 1) && Has(r, AffixKind::Mystery, 2))
    return false;
  for (std::size_t i = 0; i < state_.equipped.size(); ++i) {
    if (replace && i == *replace)
      continue;
    const auto &e = state_.equipped[i];
    if ((Has(r, AffixKind::Mystery, 1) && Has(e, AffixKind::Mystery, 2)) ||
        (Has(r, AffixKind::Mystery, 2) && Has(e, AffixKind::Mystery, 1)))
      return false;
  }
  return true;
}
void Progression::GrantRelic(Relic r) {
  if (r.id == 0)
    r.id = nextRelic_++;
  else
    nextRelic_ = std::max(nextRelic_, r.id + 1);
  auto duplicate = [&](const auto &v) {
    return std::any_of(v.begin(), v.end(),
                       [&](auto &x) { return x.id == r.id; });
  };
  if (duplicate(state_.equipped) || duplicate(state_.bag))
    return;
  if (r.catalogId >= 0 && r.catalogId < int(kRelicCount))
    state_.seenRelics[r.catalogId] = true;
  if (state_.equipped.size() < 6 && Compatible(r))
    state_.equipped.push_back(r);
  else
    state_.bag.push_back(r);
  Recompute();
}
bool Progression::Equip(EntityId id, std::size_t slot) {
  if (!resting_ || slot > state_.equipped.size() || slot >= 6)
    return false;
  auto it = std::find_if(state_.bag.begin(), state_.bag.end(),
                         [&](auto &r) { return r.id == id; });
  if (it == state_.bag.end() || !Compatible(*it, slot))
    return false;
  if (slot < state_.equipped.size())
    std::swap(*it, state_.equipped[slot]);
  else {
    state_.equipped.push_back(*it);
    state_.bag.erase(it);
  }
  Recompute();
  return true;
}
bool Progression::Unequip(std::size_t slot) {
  if (!resting_ || slot >= state_.equipped.size())
    return false;
  state_.bag.push_back(state_.equipped[slot]);
  state_.equipped.erase(state_.equipped.begin() + slot);
  Recompute();
  return true;
}
bool Progression::SetLocked(EntityId id, bool lock) {
  if (!resting_)
    return false;
  for (auto *v : {&state_.equipped, &state_.bag})
    for (auto &r : *v)
      if (r.id == id) {
        r.locked = lock;
        return true;
      }
  return false;
}
void Progression::SortBag() {
  if (!resting_)
    return;
  std::stable_sort(state_.bag.begin(), state_.bag.end(), [](auto &a, auto &b) {
    if (a.rarity != b.rarity)
      return a.rarity > b.rarity;
    if (a.catalogId != b.catalogId)
      return a.catalogId < b.catalogId;
    return a.id < b.id;
  });
}
std::vector<EntityId> Progression::FilterBag(const RelicFilter &f) const {
  std::vector<EntityId> out;
  for (auto &r : state_.bag) {
    if (f.rarity >= 0 && int(r.rarity) != f.rarity)
      continue;
    if (f.unlockedOnly && r.locked)
      continue;
    if (f.affixKind >= 0 &&
        std::none_of(r.affixes.begin(), r.affixes.end(),
                     [&](auto &a) { return int(a.kind) == f.affixKind; }))
      continue;
    if (f.aspect >= 0 &&
        std::none_of(r.affixes.begin(), r.affixes.end(), [&](auto &a) {
          return (f.aspect<int(kResonanceCount) && a.kind == AspectAffix(f.aspect)) ||
                 (a.kind == AffixKind::Resonance && a.code == f.aspect);
        }))
      continue;
    out.push_back(r.id);
  }
  return out;
}
RefinePreview
Progression::PreviewRefine(const std::vector<EntityId> &ids) const {
  RefinePreview p;
  if (!resting_)
    return p;
  std::set<EntityId> seen;
  for (auto id : ids) {
    auto i = std::find_if(state_.bag.begin(), state_.bag.end(),
                          [&](auto &r) { return r.id == id && !r.locked; });
    if (i != state_.bag.end() && seen.insert(id).second) {
      p.ids.push_back(id);
      p.xp += RefineValue(i->rarity);
    }
  }
  return p;
}
bool Progression::Refine(const std::vector<EntityId> &ids) {
  auto p = PreviewRefine(ids);
  if (p.ids.empty())
    return false;
  Undo u;
  u.xp = p.xp;
  for (auto id : p.ids) {
    auto it = std::find_if(state_.bag.begin(), state_.bag.end(),
                           [&](auto &r) { return r.id == id; });
    u.relics.push_back(*it);
    state_.bag.erase(it);
  }
  pendingRefineXp_ += p.xp;
  state_.pendingRefineXp = pendingRefineXp_;
  undo_.push_back(std::move(u));
  return true;
}
bool Progression::UndoRefine() {
  if (!resting_ || undo_.empty())
    return false;
  auto u = std::move(undo_.back());
  undo_.pop_back();
  state_.bag.insert(state_.bag.end(), u.relics.begin(), u.relics.end());
  pendingRefineXp_ -= u.xp;
  state_.pendingRefineXp = pendingRefineXp_;
  return true;
}
Stats Progression::PreviewEquip(EntityId id, std::size_t slot) const {
  auto v = state_.equipped;
  auto it = std::find_if(state_.bag.begin(), state_.bag.end(),
                         [&](auto &r) { return r.id == id; });
  if (it == state_.bag.end() || slot > v.size() || slot >= 6 ||
      !Compatible(*it, slot))
    return state_.build.stats;
  if (slot == v.size())
    v.push_back(*it);
  else
    v[slot] = *it;
  return Derive(v).stats;
}
void Progression::BeginRest() {
  if (resting_)
    return;
  resting_ = true;
  pendingRefineXp_ = 0;
  state_.pendingRefineXp = 0;
  undo_.clear();
}
void Progression::EndRest() {
  if (!resting_)
    return;
  resting_ = false;
  // Rest closes the preceding combat segment, including elite opportunities and
  // endless bosses. Their immortal reset must not suppress the next wave count.
  immortalThisWave_ = false;
  const int amount = pendingRefineXp_;
  pendingRefineXp_ = 0;
  state_.pendingRefineXp = 0;
  undo_.clear();
  AddRawXp(float(amount));
}
void Progression::SetPityWaves(int n) {
  pityWaves_ = std::max(0, n);
  state_.pityWaves = pityWaves_;
}
void Progression::OnWaveEnded() {
  if (immortalThisWave_)
    pityWaves_ = 0;
  else
    ++pityWaves_;
  immortalThisWave_ = false;
  state_.pityWaves = pityWaves_;
}
std::array<float,4> Progression::DropRarityProbabilities(bool elite) const {
  if(pityWaves_>=20)return {0,0,1,0};
  std::array<float,4> w=elite?std::array<float,4>{0,98.3f,1.5f,.2f}:std::array<float,4>{70,29.05f,.6f,.35f};
  const float luck=1+.015f*EffectiveLuck(state_.build.stats.luck);
  for(int i=elite?2:1;i<4;++i)w[i]*=luck*dropRiskWeight_;
  if(fullGame_)w[2]*=1+.1f*std::clamp(pityWaves_,0,19); // provisional gradual immortal pity
  float total=std::accumulate(w.begin(),w.end(),0.f);for(auto&x:w)x/=total;return w;
}
std::string Progression::DescribeDropOdds(bool elite) const {
  auto p=DropRarityProbabilities(elite);std::ostringstream o;o<<std::fixed<<std::setprecision(3);
  o<<(elite?"大妖/灵兽保底法宝品阶：":"随机法宝品阶（已触发掉落后）：");
  for(int i=0;i<4;++i)o<<RarityName(Rarity(i))<<" "<<p[i]*100<<"% ";
  o<<"；连续未出仙品 "<<pityWaves_<<" 波；20波下一件随机掉落必仙，道品不保底";
  if(fullGame_)o<<"；每未出仙品波仙品权重 +10%（暂定），按当前气运/劫数归一化后即上列实际概率";
  return o.str();
}
Rarity Progression::RollRarity(bool elite) {
  const bool hardPity=pityWaves_>=20;
  const auto w=DropRarityProbabilities(elite);
  float v=hardPity?0:Unit();int i=0;
  for(;i<3 && v>=w[i];++i)v-=w[i];
  if(i==2){pityWaves_=0;state_.pityWaves=0;immortalThisWave_=true;}
  return Rarity(i);
}
std::optional<Relic> Progression::OnKill(EnemyTier tier, bool beast) {
  if (tier == EnemyTier::Boss && !beast)
    return {};
  bool guaranteed = beast || tier == EnemyTier::Elite;
  if (!guaranteed && Unit() >= (tier == EnemyTier::Variant ? .025f : .005f))
    return {};
  auto relic = GenerateRelic(RollRarity(guaranteed));
  state_.seenRelics[relic.catalogId] = true;
  return relic;
}
bool Progression::RollSpiritBeast() {
  return Unit() <
         std::min(1.f,
                  .4f * (1 + .015f * EffectiveLuck(state_.build.stats.luck)));
}
bool Progression::LegalRelic(int id) const {
  if(id<0 || id>=int(kRelicCount))return false;
  if(id<15)return true;
  return fullGame_ && relicPoolUnlocked_ && (preferredRelics_.empty() || std::find(preferredRelics_.begin(),preferredRelics_.end(),id)!=preferredRelics_.end());
}
Relic Progression::GenerateRelic(Rarity r) {
  std::vector<int> pool;
  for(const auto& c:RelicCatalog())if(c.rarity==r && LegalRelic(c.id))pool.push_back(c.id);
  return GenerateRelicFromCatalog(pool[Index(int(pool.size()))]);
}
std::vector<Relic> Progression::BossRewards() {
  std::vector<int> pool;
  for(const auto&c:RelicCatalog())if(c.rarity>=Rarity::Immortal && LegalRelic(c.id))pool.push_back(c.id);
  std::vector<Relic> out;
  for (int i = 0; i < 3; ++i) {
    int k = Index(int(pool.size()));
    out.push_back(GenerateRelicFromCatalog(pool[k]));
    pool.erase(pool.begin() + k);
  }
  return out;
}
void Progression::SetDropRiskWeight(float weight){dropRiskWeight_=std::isfinite(weight)?std::clamp(weight,1.f,10.f):1.f;}
void Progression::SetRelicPool(const std::vector<int>& ids){preferredRelics_.clear();for(int id:ids)if(id>=0 && id<int(kRelicCount) && std::find(preferredRelics_.begin(),preferredRelics_.end(),id)==preferredRelics_.end())preferredRelics_.push_back(id);}
void Progression::GenerateBossRewards(std::uint64_t ordinal){
  if(ordinal==0 || ordinal<=lastBossOrdinal_)return;
  lastBossOrdinal_=ordinal;bossRewardsGenerated_=false;GenerateBossRewards();
}
void Progression::GenerateBossRewards() {
  if (!bossRewardsGenerated_) {
    bossRewardsGenerated_ = true;
    state_.bossRewards = BossRewards();
    for (const auto &r : state_.bossRewards)
      state_.seenRelics[r.catalogId] = true;
  }
}
bool Progression::ChooseBossReward(std::size_t index) {
  if (!resting_ || index >= state_.bossRewards.size())
    return false;
  auto r = state_.bossRewards[index];
  state_.bossRewards.clear();
  GrantRelic(std::move(r));
  return true;
}
Relic Progression::GenerateRelicFromCatalog(int id) {
  id = std::clamp(id, 0, int(kRelicCount)-1);
  Relic r;
  r.id = nextRelic_++;
  r.catalogId = id;
  r.rarity = RelicCatalog()[id].rarity;
  int q = int(r.rarity);
  auto add = [&](AffixKind k, int code = 0) {
    float value = 0;
    if (k <= AffixKind::Luck)
      { const auto range=AffixRange(k,r.rarity);value=range[0]+Unit()*(range[1]-range[0]); }
    else if ((k >= AffixKind::Fire && k <= AffixKind::Array) || (k>=AffixKind::Metal && k<=AffixKind::Talisman))
      value = q == 1 ? .06f : q == 2 ? .10f : .15f;
    else if (k == AffixKind::Resonance)
      value = 1;
    r.affixes.push_back({k, value, code});
  };
  if(id>=15){
    const auto& c=RelicCatalog()[id];add(c.primary);
    if(q==0){add(c.secondary);}
    else {add(c.aspect);add(AffixKind::Trigger,c.trigger);}
    if(q>=2)add(AffixKind::Mystery,c.mystery);
    if(q==3){add(AffixKind::Resonance,c.resonanceCode);add(AffixKind::Unique,c.uniqueCode);}
    return r;
  }
  switch (id) {
  case 0:
    add(AffixKind::MaxHp);
    add(AffixKind::Armor);
    break;
  case 1:
    add(AffixKind::Power);
    add(AffixKind::Crit);
    break;
  case 2:
    add(AffixKind::MoveSpeed);
    add(AffixKind::Dash);
    break;
  case 3:
    add(AffixKind::Pickup);
    add(AffixKind::Luck);
    break;
  case 4:
    add(AffixKind::Power);
    add(AffixKind::Fire);
    add(AffixKind::Trigger, 6);
    break;
  case 5:
    add(AffixKind::Crit);
    add(AffixKind::Thunder);
    add(AffixKind::Trigger, 5);
    break;
  case 6:
    add(AffixKind::Cooldown);
    add(AffixKind::Sword);
    add(AffixKind::Trigger, 1);
    break;
  case 7:
    add(AffixKind::MaxHp);
    add(AffixKind::Armor);
    add(AffixKind::Trigger, 2);
    break;
  case 8:
    add(AffixKind::Dash);
    add(AffixKind::MoveSpeed);
    add(AffixKind::Trigger, 3);
    break;
  case 9:
    add(AffixKind::Power);
    add(AffixKind::Fire);
    add(AffixKind::Trigger, 1 + Index(7));
    add(AffixKind::Mystery, 1);
    break;
  case 10:
    add(AffixKind::Crit);
    add(AffixKind::Thunder);
    add(AffixKind::Trigger, 1 + Index(7));
    add(AffixKind::Mystery, 2);
    break;
  case 11:
    add(AffixKind::Cooldown);
    add(AffixKind::Sword);
    add(AffixKind::Trigger, 1 + Index(7));
    add(AffixKind::Mystery, 3);
    break;
  case 12:
    add(AffixKind::Area);
    add(AffixKind::Array);
    add(AffixKind::Trigger, 1 + Index(7));
    add(AffixKind::Mystery, 4);
    break;
  default: {
    add(AffixKind(Index(11)));
    add(AffixKind(int(AffixKind::Fire) + Index(5)));
    add(AffixKind::Trigger, 1 + Index(7));
    std::vector<AffixKind> pool;
    for (int k = 0; k <= int(AffixKind::Resonance); ++k)
      if (std::none_of(r.affixes.begin(), r.affixes.end(),
                       [&](auto &a) { return int(a.kind) == k; }))
        pool.push_back(AffixKind(k));
    auto k = pool[Index(int(pool.size()))];
    add(k, k == AffixKind::Resonance ? Index(5) : 0);
    add(AffixKind::Mystery, id == 13 ? 6 : 5);
    add(AffixKind::Unique, id == 13 ? 2 : 1);
    break;
  }
  }
  return r;
}
} // namespace dywx::mvp
