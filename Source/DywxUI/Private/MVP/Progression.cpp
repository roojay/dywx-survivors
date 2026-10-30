#include "MVP/Progression.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>
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
  default:
    return r.array;
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
      {hero == HeroId::Body ? SpellId::FlamePalm : SpellId::FlameSword, 1});
  state_.build.passives.push_back(hero == HeroId::Body
                                      ? PassiveId::BodyScripture
                                      : PassiveId::SwordScripture);
  state_.seenSpells[int(state_.build.spells.front().id)] = true;
  Recompute();
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
  for (int i = 0; i < 10; ++i) {
    auto id = SpellId(i);
    if ((id == SpellId::FlamePalm && state_.build.hero != HeroId::Body) ||
        (id == SpellId::FlameSword && state_.build.hero != HeroId::Sword))
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
    for (int i = 2; i < 5; ++i)
      if (!HasPassive(PassiveId(i)))
        out.push_back({UpgradeKind::Passive, {}, PassiveId(i)});
  return out;
}
bool Progression::Affinity(const UpgradeOffer &o) const {
  const auto &r = state_.build.resonance;
  if (o.kind == UpgradeKind::Passive) {
    return o.passive == PassiveId::FireScripture      ? r.fire > 0
           : o.passive == PassiveId::ThunderScripture ? r.thunder > 0
                                                      : r.array > 0;
  }
  auto c = GetSpellConfig(o.spell);
  return (c.element == Element::Fire && r.fire > 0) ||
         (c.element == Element::Thunder && r.thunder > 0) ||
         (c.form == Form::Body && r.body > 0) ||
         (c.form == Form::Sword && r.sword > 0) ||
         (c.form == Form::Array && r.array > 0);
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
      SpellId a = state_.build.hero == HeroId::Body ? SpellId::FlamePalm
                                                    : SpellId::FlameSword,
              b = state_.build.hero == HeroId::Body ? SpellId::ThunderFist
                                                    : SpellId::ThunderSword;
      if (o.kind != UpgradeKind::Passive &&
          ((o.spell == a && rank5(b)) || (o.spell == b && rank5(a))))
        x *= 2;
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
  if ((result != SpellId::FusionPalm && result != SpellId::FusionSwords) ||
      (result == SpellId::FusionPalm) != (state_.build.hero == HeroId::Body) ||
      HasSpell(result))
    return false;
  auto a = result == SpellId::FusionPalm ? SpellId::FlamePalm
                                         : SpellId::FlameSword,
       b = result == SpellId::FusionPalm ? SpellId::ThunderFist
                                         : SpellId::ThunderSword;
  auto max = [&](SpellId id) {
    return std::any_of(state_.build.spells.begin(), state_.build.spells.end(),
                       [&](auto s) { return s.id == id && s.rank == 5; });
  };
  return max(a) && max(b);
}
bool Progression::Fuse(SpellId result) {
  if (!CanFuse(result) || (!resting_ && state_.offers.empty()))
    return false;
  auto a = result == SpellId::FusionPalm ? SpellId::FlamePalm
                                         : SpellId::FlameSword,
       b = result == SpellId::FusionPalm ? SpellId::ThunderFist
                                         : SpellId::ThunderSword;
  auto &v = state_.build.spells;
  v.erase(std::remove_if(v.begin(), v.end(),
                         [&](auto s) { return s.id == a || s.id == b; }),
          v.end());
  v.push_back({result, 1});
  consumed_[int(a)] = consumed_[int(b)] = true;
  state_.seenSpells[int(result)] = true;
  state_.seenFusions[result == SpellId::FusionPalm ? 0 : 1] = true;
  Recompute();
  if (!state_.offers.empty())
    MakeOffers();
  return true;
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
  b.stats = Stats{};
  b.resonance = {};
  auto &s = b.stats;
  auto &r = b.resonance;
  for (auto sp : b.spells) {
    auto c = GetSpellConfig(sp.id);
    if (sp.id == SpellId::FusionPalm || sp.id == SpellId::FusionSwords) {
      ++r.fire;
      ++r.thunder;
      Count(r, sp.id == SpellId::FusionPalm ? 2 : 3) += 2;
    } else {
      Count(r, int(c.element))++;
      Count(r, int(c.form) + 2)++;
    }
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
      case AffixKind::Resonance:
        if (!resonanceAdded && a.code >= 0 && a.code < 5) {
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
  b.stats = ClampStats(s);
  return b;
}
void Progression::Recompute() {
  state_.build = Derive(state_.equipped);
  for (auto passive : state_.build.passives)
    state_.seenPassives[static_cast<std::size_t>(passive)] = true;
  const auto &r = state_.build.resonance;
  const std::array<int, 5> counts{r.fire, r.thunder, r.body, r.sword, r.array};
  for (std::size_t i = 0; i < counts.size(); ++i)
    if (counts[i] >= 2 && (i < 2 || formResonance_))
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
  if (r.catalogId >= 0 && r.catalogId < 15)
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
          return (int(a.kind) == int(AffixKind::Fire) + f.aspect) ||
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
Rarity Progression::RollRarity(bool elite) {
  if (pityWaves_ >= 20) {
    pityWaves_ = 0;
    state_.pityWaves = 0;
    immortalThisWave_ = true;
    return Rarity::Immortal;
  }
  std::array<float, 4> w = elite ? std::array<float, 4>{0, 98.3f, 1.5f, .2f}
                                 : std::array<float, 4>{70, 29.05f, .6f, .35f};
  const float luck = 1 + .015f * EffectiveLuck(state_.build.stats.luck);
  for (int i = 1; i < 4; ++i)
    w[i] *= luck;
  float v = Unit() * std::accumulate(w.begin(), w.end(), 0.f);
  int i = 0;
  for (; i < 3 && v >= w[i]; ++i)
    v -= w[i];
  if (i == 2) {
    pityWaves_ = 0;
    state_.pityWaves = 0;
    immortalThisWave_ = true;
  }
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
Relic Progression::GenerateRelic(Rarity r) {
  std::vector<int> pool;
  for (auto &c : RelicCatalog())
    if (c.rarity == r)
      pool.push_back(c.id);
  return GenerateRelicFromCatalog(pool[Index(int(pool.size()))]);
}
std::vector<Relic> Progression::BossRewards() {
  std::vector<int> pool{9, 10, 11, 12, 13, 14};
  std::vector<Relic> out;
  for (int i = 0; i < 3; ++i) {
    int k = Index(int(pool.size()));
    out.push_back(GenerateRelicFromCatalog(pool[k]));
    pool.erase(pool.begin() + k);
  }
  return out;
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
  id = std::clamp(id, 0, 14);
  Relic r;
  r.id = nextRelic_++;
  r.catalogId = id;
  r.rarity = RelicCatalog()[id].rarity;
  int q = int(r.rarity);
  static const float low[11][4] = {{10, 20, 35, 55},
                                   {.02f, .04f, .07f, .10f},
                                   {.1f, .2f, .4f, .7f},
                                   {.04f, .07f, .12f, .18f},
                                   {.02f, .04f, .07f, .10f},
                                   {.04f, .07f, .12f, .18f},
                                   {.02f, .03f, .05f, .08f},
                                   {.02f, .03f, .05f, .08f},
                                   {.5f, 1, 1.5f, 2.5f},
                                   {.1f, .2f, .4f, .6f},
                                   {1, 2, 4, 7}};
  static const float high[11][4] = {{20, 35, 55, 80},
                                    {.04f, .07f, .10f, .14f},
                                    {.2f, .4f, .7f, 1},
                                    {.07f, .12f, .18f, .26f},
                                    {.04f, .07f, .10f, .14f},
                                    {.07f, .12f, .18f, .25f},
                                    {.03f, .05f, .08f, .12f},
                                    {.03f, .05f, .08f, .10f},
                                    {1, 1.5f, 2.5f, 3.5f},
                                    {.2f, .4f, .6f, .9f},
                                    {2, 4, 7, 10}};
  auto add = [&](AffixKind k, int code = 0) {
    float value = 0;
    if (k <= AffixKind::Luck)
      value = low[int(k)][q] + Unit() * (high[int(k)][q] - low[int(k)][q]);
    else if (k >= AffixKind::Fire && k <= AffixKind::Array)
      value = q == 1 ? .06f : q == 2 ? .10f : .15f;
    else if (k == AffixKind::Resonance)
      value = 1;
    r.affixes.push_back({k, value, code});
  };
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
