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
  // Full-game additions are provisional tuning, each row has an executable behavior.
  if (id >= SpellId::MetalPalm && id < SpellId::Count) {
    struct Row { const char* name; Element e; Form f; SpellBehavior behavior; float damage, cd, radius, duration, interval; int count, pierce; float stun, slow, heal, shield; };
    static const Row rows[] = {
      {"断金掌",Element::Metal,Form::Body,SpellBehavior::Cone,24,1.7f,3,0,0,1,1,0,0,0,0},
      {"青藤缠手",Element::Wood,Form::Body,SpellBehavior::Cone,12,1.4f,3.5f,0,0,1,1,.35f,.25f,.4f,0},
      {"沧浪掌",Element::Water,Form::Body,SpellBehavior::Cone,18,1.5f,4,0,0,1,1,0,.3f,0,0},
      {"镇岳拳",Element::Earth,Form::Body,SpellBehavior::Cone,32,2.2f,2.8f,0,0,1,1,.4f,0,0,3},
      {"庚金穿云剑",Element::Metal,Form::Sword,SpellBehavior::Projectile,20,1.6f,15,0,0,1,6,0,0,0,0},
      {"青枝回生剑",Element::Wood,Form::Sword,SpellBehavior::Projectile,10,1.8f,11,0,0,2,2,0,0,.5f,0},
      {"寒潮飞剑",Element::Water,Form::Sword,SpellBehavior::Projectile,14,1.5f,13,0,0,2,2,0,.35f,0,0},
      {"玄岩重剑",Element::Earth,Form::Sword,SpellBehavior::Projectile,38,2.8f,10,0,0,1,3,.5f,0,0,0},
      {"金罡剑阵",Element::Metal,Form::Array,SpellBehavior::Field,13,8,3,4,.7f,1,1,0,0,0,0},
      {"青木生息阵",Element::Wood,Form::Array,SpellBehavior::Field,5,10,4,6,1,1,1,0,.2f,.3f,0},
      {"寒潭阵",Element::Water,Form::Array,SpellBehavior::Field,8,9,3.5f,5,.8f,1,1,0,.4f,0,0},
      {"地阙阵",Element::Earth,Form::Array,SpellBehavior::Field,28,11,3.2f,4,1.4f,1,1,.45f,0,0,4},
      {"赤羽灵禽",Element::Fire,Form::Beast,SpellBehavior::Companion,11,7,8,6,1,1,1,0,0,0,0},
      {"惊蛰灵貂",Element::Thunder,Form::Beast,SpellBehavior::Companion,9,8,9,6,.8f,1,1,.2f,0,0,0},
      {"青鹿引灵",Element::Wood,Form::Beast,SpellBehavior::Companion,8,8,8,7,1,1,1,0,0,.8f,0},
      {"玄水灵龟",Element::Water,Form::Beast,SpellBehavior::Companion,14,10,6,8,1.5f,1,1,0,.3f,0,5},
      {"离火爆符",Element::Fire,Form::Talisman,SpellBehavior::Talisman,30,3.5f,2.3f,0,0,1,1,0,0,0,0},
      {"惊雷定符",Element::Thunder,Form::Talisman,SpellBehavior::Talisman,21,3.8f,2.8f,0,0,1,1,.7f,0,0,0},
      {"金锋破符",Element::Metal,Form::Talisman,SpellBehavior::Talisman,48,4.5f,1.8f,0,0,1,1,0,0,0,0},
      {"镇山灵符",Element::Earth,Form::Talisman,SpellBehavior::Talisman,26,4,3.4f,0,0,1,1,.5f,.3f,0,0},
      {"森罗玄武",Element::Wood,Form::Beast,SpellBehavior::Companion,28,8,10,8,.8f,2,1,.2f,.35f,1,8},
      {"离明焚天掌",Element::Fire,Form::Body,SpellBehavior::Cone,55,1.2f,4.5f,0,0,1,1,.35f,0,0,0},
      {"赤霄逐日剑",Element::Fire,Form::Sword,SpellBehavior::Projectile,24,1.1f,16,0,0,3,6,0,0,0,0},
      {"青皇瑞鹿",Element::Wood,Form::Beast,SpellBehavior::Companion,22,7,11,8,.75f,2,1,0,.25f,1.2f,5}
    };
    const auto& row=rows[int(id)-int(SpellId::MetalPalm)];
    s.name=row.name;s.element=row.e;s.form=row.f;s.behavior=row.behavior;s.provisional=true;
    s.damage=row.damage;s.cooldown=row.cd;s.radius=row.radius;s.duration=row.duration;s.interval=row.interval;
    s.count=row.count;s.pierce=row.pierce;s.stun=row.stun;s.slowFraction=row.slow;s.slowDuration=row.slow>0?2.f:0;
    s.healOnHit=row.heal;s.shieldOnCast=row.shield;
    if(s.element==Element::Fire){s.burnDps=3;s.burnDuration=3;}
    if(s.element==Element::Thunder && s.behavior==SpellBehavior::Companion){s.chains=1;s.chainScale=.5f;}
    if(s.form==Form::Body)s.coneDegrees=120;
    if(s.form==Form::Talisman)s.warning=.35f;
    if(!IsFusion(id)){
      if(rank>=2)s.damage*=1.2f;
      if(rank>=3){s.radius*=1.2f;if(s.behavior==SpellBehavior::Projectile)++s.pierce;}
      if(rank>=4){s.cooldown*=.85f;if(s.duration>0)s.duration+=1;}
      if(rank>=5){
        if(s.behavior==SpellBehavior::Companion || s.behavior==SpellBehavior::Talisman)++s.count;
        else if(s.behavior==SpellBehavior::Projectile)s.returning=true;
        else if(s.behavior==SpellBehavior::Field)s.followPlayer=true;
        else {s.coneDegrees=180;s.stun+=.25f;}
      }
    }
    return s;
  }
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
const std::array<PassiveConfig, kPassiveCount> &PassiveCatalog() {
  static const std::array<PassiveConfig, kPassiveCount> a{
      {{PassiveId::BodyScripture, "炼体真诀",
        "气血 +20，护体 +8%，法体 +1，遁息 −0.5 秒"},
       {PassiveId::SwordScripture, "御剑诀",
        "周天 +10%，威力 +8%，飞剑 +1，修为 +10%"},
       {PassiveId::FireScripture, "炼火心经",
        "威力 +12%，火 +1，灼烧伤害 +15%"},
       {PassiveId::ThunderScripture, "引雷心经",
        "会心 +8%，雷 +1，连锁伤害 +15%"},
       {PassiveId::ClearMind, "清心咒", "修为 +10%，气运 +5，阵法 +1"},
       {PassiveId::BeastScripture,"御灵真经","生机 +0.3/秒，摄灵 +1，御兽 +1，御兽威力 +10%（暂定）"},
       {PassiveId::MetalScripture,"金罡经","会心 +6%，金 +1（暂定）"},
       {PassiveId::WoodScripture,"长生经","生机 +0.4/秒，木 +1（暂定）"},
       {PassiveId::WaterScripture,"沧浪经","周天 +8%，水 +1（暂定）"},
       {PassiveId::EarthScripture,"厚土经","护体 +6%，气血 +15，土 +1（暂定）"},
       {PassiveId::TalismanScripture,"太素符经","范围 +12%，符箓 +1（暂定）"}}};
  return a;
}
const std::array<RelicConfig, kRelicCount> &RelicCatalog() {
  static const auto a=[] { std::array<RelicConfig, kRelicCount> a{
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
    // Explicit provisional themes: names, stat identities and aspect mechanics agree.
    // These are equipment template variants, not 65 new exclusive mechanisms.
    const RelicConfig additions[]={
      {15,"金砂护符",Rarity::Common,AffixKind::Armor,AffixKind::Metal,2,5,true,AffixKind::Crit,5,1,0},
      {16,"青木簪",Rarity::Common,AffixKind::Regen,AffixKind::Wood,4,6,true,AffixKind::MaxHp,6,1,2},
      {17,"寒玉佩",Rarity::Common,AffixKind::Cooldown,AffixKind::Water,2,4,true,AffixKind::Dash,7,1,0},
      {18,"镇山石",Rarity::Common,AffixKind::MaxHp,AffixKind::Earth,2,5,true,AffixKind::Armor,8,1,1},
      {19,"引兽铃",Rarity::Common,AffixKind::Pickup,AffixKind::Beast,1,4,true,AffixKind::Luck,9,1,2},
      {20,"朱砂符袋",Rarity::Common,AffixKind::Area,AffixKind::Talisman,3,6,true,AffixKind::Cooldown,10,1,1},
      {21,"赤羽护符",Rarity::Common,AffixKind::Power,AffixKind::Fire,6,1,true,AffixKind::Crit,0,2,1},
      {22,"雷纹腕环",Rarity::Common,AffixKind::Crit,AffixKind::Thunder,5,2,true,AffixKind::Dash,1,2,0},
      {23,"庚金针",Rarity::Common,AffixKind::Crit,AffixKind::Metal,5,3,true,AffixKind::Power,5,1,0},
      {24,"碧藤戒",Rarity::Common,AffixKind::MaxHp,AffixKind::Wood,4,4,true,AffixKind::Regen,6,1,2},
      {25,"清泉瓶",Rarity::Common,AffixKind::Regen,AffixKind::Water,4,6,true,AffixKind::MaxHp,7,1,0},
      {26,"黄庭印",Rarity::Common,AffixKind::Armor,AffixKind::Earth,7,5,true,AffixKind::MaxHp,8,1,1},
      {27,"灵鹿角",Rarity::Common,AffixKind::Regen,AffixKind::Beast,4,6,true,AffixKind::Pickup,9,1,2},
      {28,"太素符笔",Rarity::Common,AffixKind::Cooldown,AffixKind::Talisman,3,4,true,AffixKind::Area,10,1,2},
      {29,"火浣披风",Rarity::Spirit,AffixKind::Armor,AffixKind::Fire,7,1,true,AffixKind::Armor,0,2,1},
      {30,"惊雷腰牌",Rarity::Spirit,AffixKind::Dash,AffixKind::Thunder,3,2,true,AffixKind::Armor,1,2,0},
      {31,"金罡护腕",Rarity::Spirit,AffixKind::Armor,AffixKind::Metal,2,5,true,AffixKind::Armor,5,1,0},
      {32,"扶桑枝",Rarity::Spirit,AffixKind::Regen,AffixKind::Wood,4,1,true,AffixKind::Armor,6,1,2},
      {33,"玄冰珠",Rarity::Spirit,AffixKind::Cooldown,AffixKind::Water,2,4,true,AffixKind::Armor,7,1,0},
      {34,"厚土圭",Rarity::Spirit,AffixKind::MaxHp,AffixKind::Earth,7,5,true,AffixKind::Armor,8,1,1},
      {35,"百兽香囊",Rarity::Spirit,AffixKind::Pickup,AffixKind::Beast,1,6,true,AffixKind::Armor,9,1,2},
      {36,"天符砚",Rarity::Spirit,AffixKind::Area,AffixKind::Talisman,3,4,true,AffixKind::Armor,10,1,0},
      {37,"流火环",Rarity::Spirit,AffixKind::Power,AffixKind::Fire,6,1,true,AffixKind::Armor,0,2,1},
      {38,"电光钗",Rarity::Spirit,AffixKind::Crit,AffixKind::Thunder,5,2,true,AffixKind::Armor,1,2,0},
      {39,"白虹剑珰",Rarity::Spirit,AffixKind::Cooldown,AffixKind::Sword,1,3,true,AffixKind::Armor,3,1,0},
      {40,"回生葫芦",Rarity::Spirit,AffixKind::Regen,AffixKind::Wood,4,6,true,AffixKind::Armor,6,1,2},
      {41,"寒潮镜",Rarity::Spirit,AffixKind::Area,AffixKind::Water,2,4,true,AffixKind::Armor,7,1,0},
      {42,"镇岳冠",Rarity::Spirit,AffixKind::Armor,AffixKind::Earth,2,5,true,AffixKind::Armor,8,1,1},
      {43,"青鹿灵铃",Rarity::Spirit,AffixKind::Regen,AffixKind::Beast,4,6,true,AffixKind::Armor,9,1,2},
      {44,"飞符匣",Rarity::Spirit,AffixKind::Cooldown,AffixKind::Talisman,1,3,true,AffixKind::Armor,10,1,0},
      {45,"赤霄佩",Rarity::Spirit,AffixKind::Crit,AffixKind::Fire,6,1,true,AffixKind::Armor,0,2,1},
      {46,"惊蛰鼓",Rarity::Spirit,AffixKind::Area,AffixKind::Thunder,5,2,true,AffixKind::Armor,1,2,0},
      {47,"太白金钺",Rarity::Immortal,AffixKind::Power,AffixKind::Metal,5,5,true,AffixKind::Armor,5,1,0},
      {48,"长生玉枝",Rarity::Immortal,AffixKind::Regen,AffixKind::Wood,4,6,true,AffixKind::Armor,6,1,2},
      {49,"沧浪玉盏",Rarity::Immortal,AffixKind::Cooldown,AffixKind::Water,3,4,true,AffixKind::Armor,7,1,0},
      {50,"玄岩鼎",Rarity::Immortal,AffixKind::Armor,AffixKind::Earth,2,5,true,AffixKind::Armor,8,1,1},
      {51,"万灵笛",Rarity::Immortal,AffixKind::Pickup,AffixKind::Beast,1,6,true,AffixKind::Armor,9,1,2},
      {52,"灵篆卷",Rarity::Immortal,AffixKind::Area,AffixKind::Talisman,3,4,true,AffixKind::Armor,10,1,2},
      {53,"焚霞宝珠",Rarity::Immortal,AffixKind::Power,AffixKind::Fire,6,1,true,AffixKind::Armor,0,2,1},
      {54,"震雷法印",Rarity::Immortal,AffixKind::Crit,AffixKind::Thunder,5,2,true,AffixKind::Armor,1,2,0},
      {55,"金阙令",Rarity::Immortal,AffixKind::Armor,AffixKind::Metal,7,5,true,AffixKind::Armor,5,1,0},
      {56,"青帝木冠",Rarity::Immortal,AffixKind::MaxHp,AffixKind::Wood,4,6,true,AffixKind::Armor,6,1,2},
      {57,"北溟水盂",Rarity::Immortal,AffixKind::Regen,AffixKind::Water,4,4,true,AffixKind::Armor,7,1,0},
      {58,"后土灵璧",Rarity::Immortal,AffixKind::MaxHp,AffixKind::Earth,7,5,true,AffixKind::Armor,8,1,1},
      {59,"御灵法幡",Rarity::Immortal,AffixKind::Cooldown,AffixKind::Beast,1,4,true,AffixKind::Armor,9,1,2},
      {60,"八方符箓",Rarity::Immortal,AffixKind::Cooldown,AffixKind::Talisman,5,6,true,AffixKind::Armor,10,1,0},
      {61,"赤日轮",Rarity::Immortal,AffixKind::Area,AffixKind::Fire,6,1,true,AffixKind::Armor,0,2,1},
      {62,"天鼓雷槌",Rarity::Immortal,AffixKind::Power,AffixKind::Thunder,5,2,true,AffixKind::Armor,1,2,0},
      {63,"庚金剑胎",Rarity::Immortal,AffixKind::Cooldown,AffixKind::Sword,1,3,true,AffixKind::Armor,3,1,0},
      {64,"建木灵根",Rarity::Immortal,AffixKind::Regen,AffixKind::Wood,7,4,true,AffixKind::Armor,6,1,2},
      {65,"玄冥冰魄",Rarity::Immortal,AffixKind::Area,AffixKind::Water,2,6,true,AffixKind::Armor,7,1,0},
      {66,"五岳镇印",Rarity::Immortal,AffixKind::Armor,AffixKind::Earth,7,4,true,AffixKind::Armor,8,1,1},
      {67,"灵兽天环",Rarity::Immortal,AffixKind::Regen,AffixKind::Beast,4,6,true,AffixKind::Armor,9,1,2},
      {68,"太素天书",Rarity::Immortal,AffixKind::Luck,AffixKind::Talisman,3,6,true,AffixKind::Armor,10,1,2},
      {69,"离明天灯",Rarity::Dao,AffixKind::Power,AffixKind::Fire,6,1,true,AffixKind::Armor,0,2,1},
      {70,"紫霄雷璧",Rarity::Dao,AffixKind::Crit,AffixKind::Thunder,5,2,true,AffixKind::Armor,1,2,0},
      {71,"金华道镜",Rarity::Dao,AffixKind::Armor,AffixKind::Metal,2,5,true,AffixKind::Armor,5,1,0},
      {72,"碧落仙枝",Rarity::Dao,AffixKind::Regen,AffixKind::Wood,4,6,true,AffixKind::Armor,6,1,2},
      {73,"瀚海灵珠",Rarity::Dao,AffixKind::Cooldown,AffixKind::Water,3,4,true,AffixKind::Armor,7,1,0},
      {74,"万壑山河印",Rarity::Dao,AffixKind::MaxHp,AffixKind::Earth,7,5,true,AffixKind::Armor,8,1,1},
      {75,"森罗御灵图",Rarity::Dao,AffixKind::Regen,AffixKind::Beast,4,6,true,AffixKind::Armor,9,1,2},
      {76,"周天符命",Rarity::Dao,AffixKind::Cooldown,AffixKind::Talisman,3,6,true,AffixKind::Armor,10,1,0},
      {77,"日月同辉环",Rarity::Dao,AffixKind::Area,AffixKind::Fire,6,1,true,AffixKind::Armor,0,2,1},
      {78,"五行归元鼎",Rarity::Dao,AffixKind::Armor,AffixKind::Array,2,4,true,AffixKind::Armor,4,1,1},
      {79,"万灵归藏册",Rarity::Dao,AffixKind::Luck,AffixKind::Beast,1,6,true,AffixKind::Armor,9,1,2},
    };
    for(const auto& item:additions)a[item.id]=item;
    return a;
  }();
  return a;
}
std::string_view RarityName(Rarity r) {
  static constexpr std::array<std::string_view, 4> a{"凡品", "灵品", "仙品",
                                                     "道品"};
  return a[std::clamp(int(r), 0, 3)];
}
std::string_view AffixName(const Affix &a) {
  static constexpr std::array<std::string_view, 26> names{
      "气血",   "护体",   "生机",   "威力", "周天", "范围", "会心",
      "身法",   "摄灵",   "遁息",   "气运", "火象", "雷象", "法体象",
      "飞剑象", "阵法象", "共鸣引", "触发", "玄机", "独有", "金象", "木象", "水象", "土象", "御兽象", "符箓象"};
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
  return names[std::clamp(int(a.kind), 0, 25)];
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
                  (a.kind >= AffixKind::Fire && a.kind <= AffixKind::Array) ||
                  (a.kind >= AffixKind::Metal && a.kind <= AffixKind::Talisman));
  o << (a.kind == AffixKind::Dash ? "−" : "+") << a.value * (percent ? 100 : 1)
    << (percent ? "%" : "");
  if (a.kind == AffixKind::Resonance) {
    static const char *n[] = {"火", "雷", "法体", "飞剑", "阵法", "金", "木", "水", "土", "御兽", "符箓"};
    o << "（" << n[std::clamp(a.code, 0, 10)] << "）";
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
  if(s.slowFraction>0)o << "\n减速 " << s.slowFraction*100 << "%，" << s.slowDuration << " 秒";
  if(s.healOnHit>0)o << "\n每次命中回复气血 " << s.healOnHit;
  if(s.shieldOnCast>0)o << "\n施展获得护盾 " << s.shieldOnCast;
  if(s.behavior==SpellBehavior::Companion)o << "\n灵兽跟随修士游走，索敌范围 " << s.radius;
  if(s.behavior==SpellBehavior::Talisman)o << "\n优先追击身法最快的妖魔";
  if(s.provisional)o << "\n数值暂定，待实测校准";
  return o.str();
}
bool IsFusion(SpellId id){return id==SpellId::FusionPalm || id==SpellId::FusionSwords || id==SpellId::FusionBeast || id==SpellId::AscendantPalm || id==SpellId::AscendantSword || id==SpellId::AscendantBeast;}
const std::array<FusionRecipe,kFusionCount>& FusionCatalog(){
  static const std::array<FusionRecipe,kFusionCount> a{{
    {HeroId::Body,SpellId::FlamePalm,SpellId::ThunderFist,SpellId::FusionPalm},
    {HeroId::Sword,SpellId::FlameSword,SpellId::ThunderSword,SpellId::FusionSwords},
    {HeroId::Beast,SpellId::WoodBeast,SpellId::WaterBeast,SpellId::FusionBeast},
    {HeroId::Body,SpellId::FlamePalm,SpellId::Count,SpellId::AscendantPalm,PassiveId::FireScripture},
    {HeroId::Sword,SpellId::FlameSword,SpellId::Count,SpellId::AscendantSword,PassiveId::MetalScripture},
    {HeroId::Beast,SpellId::WoodBeast,SpellId::Count,SpellId::AscendantBeast,PassiveId::WoodScripture}}};return a;
}
int ElementResonanceCode(Element e){switch(e){case Element::Fire:return 0;case Element::Thunder:return 1;case Element::Metal:return 5;case Element::Wood:return 6;case Element::Water:return 7;case Element::Earth:return 8;default:return -1;}}
int FormResonanceCode(Form f){switch(f){case Form::Body:return 2;case Form::Sword:return 3;case Form::Array:return 4;case Form::Beast:return 9;case Form::Talisman:return 10;default:return -1;}}
AffixKind AspectAffix(int code){static const AffixKind a[]={AffixKind::Fire,AffixKind::Thunder,AffixKind::Body,AffixKind::Sword,AffixKind::Array,AffixKind::Metal,AffixKind::Wood,AffixKind::Water,AffixKind::Earth,AffixKind::Beast,AffixKind::Talisman};return a[std::clamp(code,0,10)];}
std::array<float,2> AffixRange(AffixKind kind,Rarity rarity){
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
  if(kind<AffixKind::MaxHp || kind>AffixKind::Luck)return {0,0};
  int q=std::clamp(int(rarity),0,3);return {low[int(kind)][q],high[int(kind)][q]};
}
std::string DescribeFusion(const FusionRecipe& r){
  if(r.passive!=PassiveId::Count)
    return std::string(GetSpellConfig(r.first).name)+" 五重 + 持有《"+std::string(PassiveCatalog()[int(r.passive)].name)+"》 → "+DescribeSpell(r.result,1)+"\n仅替换原术法，不占额外槽；心法保留并仅计一次道象，成品继承原术法道象计数；仅对应修士可破境（暂定路线）";
  return std::string(GetSpellConfig(r.first).name)+" 五重 + "+std::string(GetSpellConfig(r.second).name)+" 五重 → "+DescribeSpell(r.result,1)+"\n继承两门术法的全部道象计数，合并为一槽；仅对应修士可破境";
}
std::string DescribeResonance(int code){
 static const char* text[]={
 "火：两门灼烧伤害 +25%；四门灼烧最多3层，灼烧目标死亡爆炸20，半径2",
 "雷：两门命中8%引雷25；四门15%引雷25并麻痹0.5秒",
 "法体：两门护体 +10%、法体威力 +15%；四门近身命中10%获得1秒无敌，冷却5秒",
 "飞剑：两门数量或穿透 +1；四门每8秒，3秒内飞剑伤害 +30%",
 "阵法：两门持续 +25%；四门阵域内妖魔承伤 +20%",
 "金：两门金象威力 +15%；四门额外会心 +10%（暂定）",
 "木：两门生机 +0.5/秒；四门额外生机 +0.5/秒、木象威力 +20%（暂定）",
 "水：两门水象威力 +15%；四门额外周天 +10%（暂定）",
 "土：两门护体 +8%；四门额外气血 +30、土象威力 +20%（暂定）",
 "御兽：两门御兽威力 +15%；四门额外御兽威力 +15%、生机 +0.8/秒（暂定）",
 "符箓：两门符箓威力 +15%；四门额外符箓威力 +15%、范围 +20%（暂定）"};
 return text[std::clamp(code,0,int(kResonanceCount)-1)];
}
std::string DescribeRelic(int id){
 id=std::clamp(id,0,int(kRelicCount)-1);const auto& c=RelicCatalog()[id];
 std::ostringstream o;o<<c.name<<" · "<<RarityName(c.rarity)<<"\n";
 static const char* legacy[]={
 "主题：气血、护体","主题：威力、会心","主题：身法、遁息","主题：摄灵、气运",
 "主题：威力、火象、焚余","主题：会心、雷象、引雷","主题：周天、飞剑象、逐风",
 "主题：气血、护体、凝盾","主题：遁息、身法、破空","主题：威力、火象、随机触发、余烬遁",
 "主题：会心、雷象、随机触发、惊雷遁","主题：周天、飞剑象、随机触发、剑分化",
 "主题：范围、阵法象、随机触发、阵随身","主题：随机属性、道象、触发及额外灵纹；会心通脉、万象同源",
 "主题：随机属性、道象、触发及额外灵纹；震劲、周天归一"};
 if(id<15)o<<legacy[id];else{
   o<<"主题："<<AffixName({c.primary,0,0});
   if(c.rarity==Rarity::Common)o<<"、"<<AffixName({c.secondary,0,0});
   else o<<"、"<<AffixName({c.aspect,0,0})<<"\n"<<DescribeAffix({AffixKind::Trigger,0,c.trigger});
   if(c.rarity>=Rarity::Immortal)o<<"\n"<<DescribeAffix({AffixKind::Mystery,0,c.mystery});
   if(c.rarity==Rarity::Dao)o<<"\n"<<DescribeAffix({AffixKind::Resonance,1,c.resonanceCode})<<"\n"<<DescribeAffix({AffixKind::Unique,0,c.uniqueCode});
 }
 o<<"\n本品阶属性灵纹范围（仅主题或随机抽中者生效）：";
 for(int i=0;i<11;++i){auto k=AffixKind(i);auto r=AffixRange(k,c.rarity);o<<"\n"<<DescribeAffix({k,r[0],0})<<" 至 "<<DescribeAffix({k,r[1],0});}
 if(c.rarity>=Rarity::Spirit)o<<"\n道象伤害灵纹 +"<<(c.rarity==Rarity::Spirit?6:c.rarity==Rarity::Immortal?10:15)<<"%";
 if(id<15 && c.rarity>=Rarity::Spirit){for(int n=1;n<=7;++n)o<<"\n"<<DescribeAffix({AffixKind::Trigger,0,n});}
 if(id<15 && c.rarity>=Rarity::Immortal){for(int n=1;n<=6;++n)o<<"\n"<<DescribeAffix({AffixKind::Mystery,0,n});}
 if(id==13 || id==14)o<<"\n"<<DescribeAffix({AffixKind::Unique,0,id==13?2:1});
 if(c.realm>=0){static const char* names[]={"雷泽洞天","赤山洞天","青岚洞天"};o<<"\n专属掉落："<<names[c.realm]<<"（需传承解锁新增法宝池）";}else o<<"\n三洞天共享掉落池";
 if(c.provisional)o<<"\n新增主题与数值暂定，待实测校准";
 return o.str();
}
std::vector<int> RealmRelicPool(int realm){std::vector<int> ids;for(const auto&c:RelicCatalog())if(c.realm==realm)ids.push_back(c.id);return ids;}
float EffectiveLuck(float luck) {
  return 40.f * (1.f - std::exp(-std::max(0.f, luck) / 30.f));
}
int RefineValue(Rarity r) {
  static constexpr int a[] = {4, 12, 40, 100};
  return a[std::clamp(int(r), 0, 3)];
}
} // namespace dywx::mvp
