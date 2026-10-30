#include "MVP/ContentCatalog.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace dywx::mvp {
SpellConfig GetSpellConfig(SpellId id, int rank) {
  rank = std::clamp(rank, 1, 5);
  SpellConfig s;
  s.id = id;
  switch (id) {
  case SpellId::FlamePalm:
    s.name = "焚心掌";
    s.element = Element::Fire;
    s.form = Form::Body;
    s.damage = 16;
    s.radius = 3;
    s.coneDegrees = 120;
    s.burnDps = 3;
    s.burnDuration = 3;
    if (rank >= 3)
      s.coneDegrees = 150;
    if (rank >= 4) {
      s.burnDuration = 5;
      s.burnDps *= 1.3f;
    }
    s.fireAfterwave = rank == 5;
    break;
  case SpellId::ThunderFist:
    s.name = "惊雷拳";
    s.element = Element::Thunder;
    s.form = Form::Body;
    s.damage = 20;
    s.cooldown = 1.4f;
    s.radius = 2.2f;
    s.coneDegrees = 120;
    s.chains = rank >= 3 ? 3 : 2;
    s.chainScale = rank >= 4 ? .75f : .6f;
    s.highestHpChain = rank == 5;
    break;
  case SpellId::FlameBody:
    s.name = "炎身诀";
    s.element = Element::Fire;
    s.form = Form::Body;
    s.damage = 5;
    s.interval = rank >= 4 ? .4f : .5f;
    s.cooldown = s.interval;
    s.radius = rank >= 3 ? 3.2f : 2.5f;
    s.burnDps = 3;
    s.burnDuration = 3;
    s.followPlayer = true;
    s.onHitExplosion = rank == 5;
    break;
  case SpellId::FlameSword:
    s.name = "赤焰飞剑";
    s.element = Element::Fire;
    s.form = Form::Sword;
    s.damage = 14;
    s.cooldown = 1.1f;
    s.radius = 12;
    s.count = rank >= 3 ? 2 : 1;
    s.pierce = rank >= 4 ? 5 : 3;
    s.burnDps = 2;
    s.burnDuration = 2;
    s.returning = rank == 5;
    break;
  case SpellId::FlameRing:
    s.name = "离火剑环";
    s.element = Element::Fire;
    s.form = Form::Sword;
    s.damage = 6;
    s.interval = .5f;
    s.cooldown = .5f;
    s.radius = 2.8f;
    s.count = rank >= 3 ? 3 : 2;
    s.rotationMultiplier = rank >= 4 ? 1.25f : 1;
    s.expandingRing = rank == 5;
    break;
  case SpellId::ThunderSword:
    s.name = "紫电飞剑";
    s.element = Element::Thunder;
    s.form = Form::Sword;
    s.damage = 12;
    s.cooldown = 1.3f;
    s.radius = 14;
    s.chains = rank >= 3 ? 3 : 2;
    s.chainScale = .5f;
    s.turnMultiplier = rank >= 4 ? 1.5f : 1;
    s.count = rank == 5 ? 2 : 1;
    break;
  case SpellId::SwordRain:
    s.name = "九霄剑雨";
    s.element = Element::Thunder;
    s.form = Form::Sword;
    s.damage = 10;
    s.cooldown = 4;
    s.radius = rank >= 4 ? 5 : 4;
    s.count = rank >= 3 ? 12 : 8;
    s.warning = .8f;
    s.electricAfterfield = rank == 5;
    break;
  case SpellId::FlameArray:
    s.name = "烈焰阵";
    s.element = Element::Fire;
    s.form = Form::Array;
    s.damage = 8;
    s.cooldown = 8;
    s.duration = rank >= 4 ? 7 : 5;
    s.interval = .5f;
    s.radius = rank >= 3 ? 3.8f : 3;
    s.burnDps = 3;
    s.burnDuration = 3;
    s.followPlayer = rank == 5;
    break;
  case SpellId::ThunderPrison:
    s.name = "雷牢阵";
    s.element = Element::Thunder;
    s.form = Form::Array;
    s.damage = 10;
    s.cooldown = 9;
    s.duration = 4;
    s.interval = 1.2f;
    s.radius = rank >= 4 ? 4.5f : 3.5f;
    s.stun = rank >= 3 ? .9f : .6f;
    s.prisonVulnerability = rank == 5;
    break;
  case SpellId::HeavenlyThunder:
    s.name = "天雷阵";
    s.element = Element::Thunder;
    s.form = Form::Array;
    s.damage = 160;
    s.cooldown = rank >= 4 ? 10 : 12;
    s.radius = 5;
    s.warning = rank >= 3 ? 1.5f : 2;
    s.afterBolts = rank == 5 ? 2 : 0;
    break;
  case SpellId::FusionPalm:
    s.name = "雷火焚天掌";
    s.element = Element::Fire;
    s.form = Form::Body;
    s.damage = 90;
    s.cooldown = 1.4f;
    s.radius = 4;
    s.burnDps = 3;
    s.burnDuration = 4;
    s.stun = .8f;
    break;
  case SpellId::FusionSwords:
    s.name = "雷火双剑";
    s.element = Element::Fire;
    s.form = Form::Sword;
    s.damage = 22;
    s.cooldown = 1.2f;
    s.radius = 14;
    s.count = 2;
    s.chains = 2;
    s.chainScale = .5f;
    s.burnDps = 2;
    s.burnDuration = 2;
    break;
  default:
    s.name = "未知术法";
    break;
  }
  if (rank >= 2 && id < SpellId::FusionPalm)
    s.damage *= 1.2f;
  return s;
}
const std::array<PassiveConfig, 5> &PassiveCatalog() {
  static const std::array<PassiveConfig, 5> a{
      {{PassiveId::BodyScripture, "炼体真诀",
        "气血 +20，护体 +8%，法体 +1，遁息 −0.5 秒"},
       {PassiveId::SwordScripture, "御剑诀",
        "周天 +10%，威力 +8%，飞剑 +1，修为 +10%"},
       {PassiveId::FireScripture, "炼火心经",
        "威力 +12%，火 +1，灼烧伤害 +15%"},
       {PassiveId::ThunderScripture, "引雷心经",
        "会心 +8%，雷 +1，连锁伤害 +15%"},
       {PassiveId::ClearMind, "清心咒", "修为 +10%，气运 +5，阵法 +1"}}};
  return a;
}
const std::array<RelicConfig, 15> &RelicCatalog() {
  static const std::array<RelicConfig, 15> a{
      {{0, "护心铜镜", Rarity::Common},
       {1, "桃木符牌", Rarity::Common},
       {2, "云纹草履", Rarity::Common},
       {3, "采灵玉囊", Rarity::Common},
       {4, "赤铜火铃", Rarity::Spirit},
       {5, "惊蛰雷珠", Rarity::Spirit},
       {6, "青锋剑穗", Rarity::Spirit},
       {7, "玄龟护腕", Rarity::Spirit},
       {8, "流云披帛", Rarity::Spirit},
       {9, "离火金乌坠", Rarity::Immortal},
       {10, "九曲雷纹符", Rarity::Immortal},
       {11, "太乙剑匣", Rarity::Immortal},
       {12, "八卦阵盘", Rarity::Immortal},
       {13, "混元珠", Rarity::Dao},
       {14, "周天星盘", Rarity::Dao}}};
  return a;
}
std::string_view RarityName(Rarity r) {
  static constexpr std::array<std::string_view, 4> a{"凡品", "灵品", "仙品",
                                                     "道品"};
  return a[std::clamp(int(r), 0, 3)];
}
std::string_view AffixName(const Affix &a) {
  static constexpr std::array<std::string_view, 20> names{
      "气血",   "护体",   "生机",   "威力", "周天", "范围", "会心",
      "身法",   "摄灵",   "遁息",   "气运", "火象", "雷象", "法体象",
      "飞剑象", "阵法象", "共鸣引", "触发", "玄机", "独有"};
  static constexpr std::array<std::string_view, 7> t{
      "逐风", "凝盾", "破空", "回春", "引雷", "焚余", "守元"};
  static constexpr std::array<std::string_view, 6> m{
      "余烬遁", "惊雷遁", "剑分化", "阵随身", "震劲", "会心通脉"};
  if (a.kind == AffixKind::Trigger && a.code >= 1 && a.code <= 7)
    return t[a.code - 1];
  if (a.kind == AffixKind::Mystery && a.code >= 1 && a.code <= 6)
    return m[a.code - 1];
  if (a.kind == AffixKind::Unique)
    return a.code == 1 ? "周天归一" : "万象同源";
  return names[std::clamp(int(a.kind), 0, 19)];
}
std::string DescribeAffix(const Affix &a) {
  static const std::array<const char *, 7> t{
      "击杀后 2 秒身法 +15%",
      "受击 8% 获得 20 护盾 4 秒；冷却 8 秒",
      "遁术后 2 秒威力 +15%",
      "每击杀 25 只回复 5% 气血",
      "会心 10% 引雷 15；冷却 1 秒",
      "灼烧死亡 10% 爆炸 12，半径 1.5",
      "气血低于 30% 护体 +15%，到 50% 结束；每波一次"};
  static const std::array<const char *, 6> m{
      "遁术起点留火 3 秒，8/秒，半径 2；与惊雷遁互斥",
      "遁术终点落雷 40，半径 2；与余烬遁互斥",
      "飞剑每第 5 次发射分出 2 柄，伤害 60%",
      "阵域随身，范围 −20%",
      "近身命中 12% 追加冲击波 30，半径 3",
      "会心缩短其他术法冷却 0.3 秒；冷却 0.5 秒"};
  std::string s(AffixName(a));
  s += "：";
  if (a.kind == AffixKind::Trigger && a.code >= 1 && a.code <= 7)
    return s + t[a.code - 1];
  if (a.kind == AffixKind::Mystery && a.code >= 1 && a.code <= 6)
    return s + m[a.code - 1];
  if (a.kind == AffixKind::Unique)
    return s +
           (a.code == 1 ? "每 20 秒重置所有术法冷却" : "火、雷共鸣计数各 +1");
  std::ostringstream o;
  o << s << std::fixed << std::setprecision(1);
  bool percent = (a.kind == AffixKind::Armor || a.kind == AffixKind::Power ||
                  a.kind == AffixKind::Cooldown || a.kind == AffixKind::Area ||
                  a.kind == AffixKind::Crit || a.kind == AffixKind::MoveSpeed ||
                  (a.kind >= AffixKind::Fire && a.kind <= AffixKind::Array));
  o << (a.kind == AffixKind::Dash ? "−" : "+") << a.value * (percent ? 100 : 1)
    << (percent ? "%" : "");
  if (a.kind == AffixKind::Resonance) {
    static const char *n[] = {"火", "雷", "法体", "飞剑", "阵法"};
    o << "（" << n[std::clamp(a.code, 0, 4)] << "）";
  }
  return o.str();
}
std::string DescribeSpell(SpellId id, int rank) {
  const auto s = GetSpellConfig(id, rank);
  std::ostringstream o;
  o << s.name << " · " << rank << " 重\n威力 " << s.damage << " · 周期 "
    << s.cooldown << " 秒 · 范围 " << s.radius;
  if (s.count > 1)
    o << "\n数量 " << s.count;
  if (s.pierce > 1)
    o << " · 穿透 " << s.pierce;
  if (s.coneDegrees < 360)
    o << " · 扇面 " << s.coneDegrees << "°";
  if (s.duration > 0)
    o << "\n持续 " << s.duration << " 秒，每 " << s.interval << " 秒一跳";
  if (s.warning > 0)
    o << "\n蓄势 " << s.warning << " 秒";
  if (s.burnDps > 0)
    o << "\n灼烧 " << s.burnDps << "/秒，" << s.burnDuration << " 秒";
  if (s.chains > 0)
    o << "\n连锁 " << s.chains << " 次（" << s.chainScale * 100
      << "%），连锁范围 " << s.chainRadius;
  if (s.stun > 0)
    o << "\n麻痹 " << s.stun << " 秒";
  if (s.fireAfterwave)
    o << "\n命中留下余波 2 秒，4/秒";
  if (s.highestHpChain)
    o << "\n连锁优先气血最高的妖魔";
  if (s.onHitExplosion)
    o << "\n受击爆燃 40，半径 3，冷却 6 秒";
  if (s.returning)
    o << "\n飞剑回旋，再次穿透";
  if (s.expandingRing)
    o << "\n每 6 秒扩展至半径 5，持续 1.5 秒";
  if (s.turnMultiplier > 1)
    o << "\n追踪转向 +50%";
  if (s.rotationMultiplier > 1)
    o << "\n转速 +25%";
  if (s.electricAfterfield)
    o << "\n剑雨后留电域 2 秒，5/秒";
  if (s.followPlayer && s.form == Form::Array)
    o << "\n阵域随身";
  if (s.prisonVulnerability)
    o << "\n阵内麻痹妖魔受到伤害 +20%";
  if (s.afterBolts > 0)
    o << "\n余雷 " << s.afterBolts << " 道，各 " << s.afterBoltScale * 100
      << "% 威力";
  return o.str();
}
float EffectiveLuck(float luck) {
  return 40.f * (1.f - std::exp(-std::max(0.f, luck) / 30.f));
}
int RefineValue(Rarity r) {
  static constexpr int a[] = {4, 12, 40, 100};
  return a[std::clamp(int(r), 0, 3)];
}
} // namespace dywx::mvp
