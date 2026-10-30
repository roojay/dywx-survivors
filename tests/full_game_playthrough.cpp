// Full-game natural smoke: keyboard-like inputs and legal UI-facing commands only.
// Policy adapted from mvp_playthrough; no benchmark or harness mutation paths.
#include "MVP/Simulation.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace dywx::mvp;
static float len(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
static Vec2 unit(Vec2 v) { return v * (1 / std::max(.001f, len(v))); }
static int rank(const BuildState &b, SpellId id) {
  for (auto s : b.spells)
    if (s.id == id)
      return s.rank;
  return 0;
}
static std::string build(const BuildState &b) {
  std::ostringstream o;
  for (auto s : b.spells)
    o << int(s.id) << ":" << s.rank << ";";
  o << "P";
  for (auto p : b.passives)
    o << int(p) << ";";
  return o.str();
}
static int value(const UpgradeOffer &o, const BuildState &b, int policy) {
  auto first =
      b.hero == HeroId::Body ? SpellId::FlamePalm : b.hero == HeroId::Beast ? SpellId::WoodBeast : SpellId::FlameSword;
  auto second =
      b.hero == HeroId::Body ? SpellId::ThunderFist : b.hero == HeroId::Beast ? SpellId::WaterBeast : SpellId::ThunderSword;
  if (o.kind == UpgradeKind::Passive)
    return o.passive == PassiveId::FireScripture ? 65
           : o.passive == PassiveId::ClearMind   ? 35
                                                 : 30;
  int v = 20;
  if (o.spell == SpellId::FlameBody)
    v = o.kind == UpgradeKind::NewSpell ? 110 : 60;
  if (o.spell == SpellId::FlameArray)
    v = o.kind == UpgradeKind::NewSpell ? 90 : 45;
  if (o.spell == SpellId::FlameRing)
    v = o.kind == UpgradeKind::NewSpell ? 85 : 40;
  if (o.spell == first || o.spell == second)
    v = 75 + (rank(b, first) + rank(b, second) >= 6 ? 25 : 0);
  if (o.spell == SpellId::HeavenlyThunder)
    v = 55;
  if (o.spell == SpellId::SwordRain)
    v = 50;
  if (policy == 1 && (o.spell == first || o.spell == second))
    v += 30;
  if (policy == 2 && o.kind == UpgradeKind::NewSpell)
    v += 25;
  return v;
}
static void upgrades(Simulation &s, int policy, std::string &log) {
  auto &p = s.GetProgression();
  auto f = p.GetState().build.hero == HeroId::Body ? SpellId::FusionPalm
                                                   : p.GetState().build.hero == HeroId::Beast ? SpellId::FusionBeast : SpellId::FusionSwords;
  if (p.CanFuse(f) && p.Fuse(f)) {
    log += "F" + std::to_string(int(f)) + ";";
    s.RefreshProgression();
  }
  if (s.GetSnapshot().progression.offers.empty())
    return;
  for (;;) {
    auto ps = s.GetSnapshot().progression;
    size_t pick = 0;
    int best = -1;
    for (size_t i = 0; i < ps.offers.size(); ++i) {
      int v = value(ps.offers[i], ps.build, policy);
      if (v > best) {
        best = v;
        pick = i;
      }
    }
    if (best < 65 && ps.rerollsRemaining > 0) {
      if (!s.Reroll())
        throw std::runtime_error("reroll rejected");
      log += "R;";
      continue;
    }
    auto o = ps.offers[pick];
    log += std::to_string(int(o.kind)) + ":" +
           std::to_string(o.kind == UpgradeKind::Passive ? int(o.passive)
                                                         : int(o.spell)) +
           ";";
    if (!s.ChooseUpgrade(pick))
      throw std::runtime_error("choice rejected");
    break;
  }
}
static float equipmentValue(const Stats &s) {
  return s.maxHp * .05f + s.armor * 35 + s.regen * 15 + s.power * 20 +
         s.cooldownReduction * 45 + s.moveSpeed * 3 + s.area * 8 +
         s.pickupRadius + s.critChance * 12 + (s.triggers[3] ? 15 : 0);
}
static float segment(Vec2 p, Vec2 a, Vec2 b) {
  Vec2 v = b - a;
  float dot = (p.x - a.x) * v.x + (p.y - a.y) * v.y;
  float t = std::clamp(dot / std::max(.001f, v.x * v.x + v.y * v.y), 0.f, 1.f);
  return len(p - (a + v * t));
}
static InputState decide(const Snapshot &s, const ArenaLayout& arena, int policy) {
  Vec2 p = s.player.position;
  const EnemyState *target = nullptr;
  float nearest = 1e6f;
  const EnemyState *boss = nullptr;
  for (auto &e : s.enemies) {
    float d = len(e.position - p);
    if (d < nearest) {
      nearest = d;
      target = &e;
    }
    if (e.role == EnemyRole::Boss)
      boss = &e;
  }
  if (boss)
    target = boss;
  bool melee = s.progression.build.hero == HeroId::Body;
  float desired = boss ? (melee ? 3.f : 6.f) : (melee ? 2.5f : 4.f);
  if (policy == 3 && !melee)
    desired = boss ? 3.2f : 2.9f;
  if (policy == 4 && !melee)
    desired = boss ? 4.f : 3.3f;
  if (rank(s.progression.build, SpellId::FusionPalm))
    desired = 3.4f;
  Vec2 goal = target ? target->position : p;
  float pickupDist = 7;
  if (!boss && nearest > 4)
    for (auto &x : s.pickups) {
      float d = len(x.position - p);
      if (d < pickupDist) {
        pickupDist = d;
        goal = x.position;
      }
    }
  Vec2 circle = unit(Vec2{-p.y / 12, p.x / 22});
  if (len(circle) < .1f)
    circle = {1, 0};
  float best = -1e20f;
  InputState result;
  // Evaluate hold plus 8 keyboard-direction headings. No teleport or separate
  // aim.
  for (int dash = 0; dash < 2; ++dash) {
    if (dash && (s.player.dashCooldown > 0 || nearest > 4))
      continue;
    for (int i = -1; i < 8; ++i) {
      Vec2 move = i < 0 ? Vec2{}
                        : Vec2{std::cos(i * 6.2831853f / 8),
                               std::sin(i * 6.2831853f / 8)};
      float score = 0;
      for (float t : {.15f, .4f, .7f}) {
        Vec2 q = p + move * (dash ? 4.f + s.progression.build.stats.moveSpeed *
                                              std::max(0.f, t - .3f)
                                  : s.progression.build.stats.moveSpeed * t);
        score -= std::max(0.f, std::abs(q.x) - 36) * 6 +
                 std::max(0.f, std::abs(q.y) - 19) * 8;
        for (auto o : arena.obstacles)
          score -= std::max(0.f, o.radius + .7f - len(q - o.position)) * 25;
        for (auto &e : s.enemies) {
          if (e.role == EnemyRole::Boss)
            continue;
          Vec2 ep = e.position;
          if (e.role == EnemyRole::Swarm || e.role == EnemyRole::Exploder)
            ep = ep + unit(p - ep) * e.speed * t;
          float d = len(q - ep);
          score -= std::max(0.f, 2.3f - d) * 14;
          if (d < e.radius + .65f)
            score -= 35;
        }
        for (auto &fx : s.effects)
          if (fx.hostile && fx.warning < t + .3f) {
            float d = fx.kind == EffectKind::Line
                          ? segment(q, fx.position, fx.end)
                          : len(q - fx.position);
            if (fx.kind == EffectKind::FireWall) {
              if (std::abs(q.x) > 19 || std::abs(q.y) > 12)
                score -= 70;
            } else
              score -= std::max(0.f, fx.radius + .7f - d) * 35;
          }
        for (auto &pr : s.projectiles)
          if (pr.hostile)
            score -=
                std::max(0.f, .9f - segment(q, pr.position,
                                            pr.position + pr.velocity * t)) *
                30;
        if (target) {
          float d = len(q - goal);
          score -= std::abs(d - desired) * (boss ? 7.f : 1.5f);
        }
      }
      if (!boss)
        score += (move.x * circle.x + move.y * circle.y) * 3;
      // Facing is held when no key is pressed. Read current cooldown cues,
      // never set them.
      Vec2 facing = i < 0 ? s.player.facing : move;
      if (target && melee) {
        Vec2 to = unit(target->position - p);
        float dot = to.x * facing.x + to.y * facing.y;
        for (auto sp : s.progression.build.spells)
          if ((sp.id == SpellId::FlamePalm || sp.id == SpellId::ThunderFist) &&
              s.spellCooldowns[int(sp.id)] < .15f &&
              len(target->position - p) <
                  GetSpellConfig(sp.id, sp.rank).radius *
                          s.progression.build.stats.area +
                      target->radius)
            score += dot > .55f ? 30 : -20;
      }
      if (dash)
        score -= 8;
      if (policy == 2 && i < 0)
        score += 1;
      if (score > best) {
        best = score;
        result = {move, bool(dash)};
      }
    }
  }
  return result;
}
struct Actions { int rest=0, opportunities=0, buys=0, reveals=0, identifies=0, equips=0, refines=0, reforges=0; };
static void arrange(Simulation& s, Actions& a) {
  auto& p=s.GetProgression();
  auto owned=p.GetState().equipped;owned.insert(owned.end(),p.GetState().bag.begin(),p.GetState().bag.end());
  for(const auto& r:owned) if(s.GetEconomy().IsUnidentified(r.id)&&s.IdentifyRelic(r.id))++a.identifies;
  for(int pass=0;pass<6;++pass) {
    EntityId best=0;std::size_t slot=p.GetState().equipped.size();
    float score=equipmentValue(p.GetState().build.stats);
    const auto bag=p.GetState().bag;
    for(const auto& r:bag) {
      if(s.GetEconomy().IsUnidentified(r.id))continue;
      if(p.GetState().equipped.size()<6){best=r.id;break;}
      for(std::size_t i=0;i<p.GetState().equipped.size();++i) {
        float v=equipmentValue(p.PreviewEquip(r.id,i));
        if(v>score+.01f){score=v;best=r.id;slot=i;}
      }
    }
    if(!best||!p.Equip(best,slot))break;
    ++a.equips;s.RefreshProgression();
  }
  std::vector<EntityId> refine;
  for(const auto& r:p.GetState().bag)if(!r.locked&&!s.GetEconomy().IsUnidentified(r.id))refine.push_back(r.id);
  if(!refine.empty()&&s.Refine(refine,RefineReward::Experience))++a.refines;
}
static bool restActions(Simulation& s, Actions& a, std::string& choices) {
  ++a.rest;
  auto cs=s.GetCampaign().GetState();
  if(cs.offerCount&&cs.selected<0) {
    // Predeclared deterministic rotation, not outcome-based seed searching.
    auto desired=OpportunityKind((a.opportunities+1)%4);
    unsigned pick=0;for(unsigned i=0;i<cs.offerCount;++i)if(cs.offers[i]==desired)pick=i;
    if(!s.ChooseOpportunity(pick))throw std::runtime_error("legal opportunity rejected");
    ++a.opportunities;
    if(s.GetSnapshot().phase!=RunPhase::Rest)return true;
  }
  if(s.GetEconomy().HasTreasureToReveal()) { if(!s.RevealTreasure())throw std::runtime_error("treasure reveal rejected");++a.reveals; }
  if(!s.GetSnapshot().progression.bossRewards.empty()&&!s.ChooseBossReward(0))throw std::runtime_error("boss choice rejected");
  auto stock=s.GetEconomy().Shop();
  // One affordable item each rest, prefer higher rarity then lowest token.
  std::stable_sort(stock.begin(),stock.end(),[](const auto& x,const auto& y){return x.relic.rarity>y.relic.rarity;});
  for(const auto& g:stock)if(!g.sold&&s.GetEconomy().Stones()>=g.price) {if(s.BuyGood(g.token))++a.buys;break;}
  arrange(s,a);
  // Exercise reforge only if the actual cave and wallet permit it; keep the old affix.
  for(const auto& r:s.GetSnapshot().progression.equipped) {
    bool done=false;
    for(std::size_t i=0;i<r.affixes.size();++i)if(s.BeginReforge(r.id,i)) {
      if(!s.ResolveReforge(false))throw std::runtime_error("reforge resolution rejected");
      ++a.reforges;done=true;break;
    }
    if(done)break;
  }
  if(!s.FinalizeRestExperience())throw std::runtime_error("rest XP commit rejected");
  for(int i=0;i<100 && !s.GetSnapshot().progression.offers.empty();++i)upgrades(s,0,choices);
  if(!s.BeginNextWave())throw std::runtime_error("unexplained intermission block");
  return true;
}
static void run(unsigned seed,HeroId hero,CaveId cave,GameMode mode,unsigned ordinal,std::ostream& trace,bool novice=false) {
  // Account-only fixture: three tiers/content unlocked; zero permanent stat ranks,
  // zero starting currency/items/XP; risk0. No run-state grants or damage hooks.
  CampaignProfile profile;
  if(!novice){profile.anyWin=profile.traditionalWin=true;profile.unlocks.fill(true);profile.unlockedRisk=4;}
  auto id=ReserveRunId(profile);
  Simulation s;
  if(!s.StartCampaign(profile,mode,hero,seed,id,cave))throw std::runtime_error("campaign entry rejected");
  const int bound=mode==GameMode::Quick?1200:mode==GameMode::Traditional?2400:1200;
  const int maxTicks=bound*60;
  std::string choices;Actions actions;InputState input;int peak=0,lastWave=-1,lastPhase=-1;
  float nextTrace=0,bossEntered=-1,bossHp=0;int bossPhase=0;unsigned lastTick=0,stalled=0;
  for(int tick=0;tick<maxTicks;++tick) {
    const auto& x=s.GetSnapshot();
    if(x.phase==RunPhase::Dead||x.phase==RunPhase::Won)break;
    upgrades(s,0,choices);
    if(s.GetSnapshot().phase==RunPhase::Rest)restActions(s,actions,choices);
    if(tick%6==0)input=decide(s.GetSnapshot(),s.GetArena(),0);
    s.Step(1.f/60,input);
    const auto& n=s.GetSnapshot();peak=std::max(peak,int(n.enemies.size()));
    for(const auto& enemy:n.enemies)if(enemy.role==EnemyRole::Boss){if(bossEntered<0)bossEntered=n.statistics.elapsed;bossHp=enemy.hp;bossPhase=enemy.bossPhase;}
    if(!std::isfinite(n.player.hp)||!std::isfinite(n.statistics.damageDealt)||n.enemies.size()>s.GetConfig().maxEnemies||n.progression.build.spells.size()>6||n.progression.build.passives.size()>4)
      throw std::runtime_error("natural invariant");
    if(n.tick==lastTick)++stalled;else stalled=0;lastTick=unsigned(n.tick);
    if(stalled>600)throw std::runtime_error("simulation tick stalled for ten seconds of input");
    if(n.statistics.elapsed>=nextTrace||n.wave!=lastWave||int(n.phase)!=lastPhase) {
      trace<<ordinal<<','<<seed<<','<<int(mode)<<','<<int(hero)<<','<<int(cave)<<','<<n.statistics.elapsed<<','<<n.wave<<','<<int(n.phase)<<','<<n.player.hp<<','<<n.progression.level<<','<<n.statistics.kills<<','<<s.GetEconomy().Stones()<<','<<n.enemies.size()<<'\n';
      nextTrace=n.statistics.elapsed+30;lastWave=n.wave;lastPhase=int(n.phase);
    }
  }
  const auto& x=s.GetSnapshot();const char* outcome=x.phase==RunPhase::Won?"won":x.phase==RunPhase::Dead?"dead":"sample_bound";
  bool settled=s.SettleCampaign(profile);
  int immortal=0,dao=0;for(const auto& relic:x.progression.equipped){immortal+=relic.rarity==Rarity::Immortal;dao+=relic.rarity==Rarity::Dao;}
  std::cout<<seed<<','<<int(mode)<<','<<int(hero)<<','<<int(cave)<<','<<(novice?"fresh_tier1_no_unlocks":"veteran_unlocks_zero_stats")<<",0,"<<bound<<','<<outcome<<','<<x.statistics.elapsed<<','<<x.wave<<','<<x.player.hp<<','<<x.statistics.kills<<','<<x.progression.level<<','<<peak<<','<<actions.opportunities<<','<<actions.buys<<','<<actions.reveals<<','<<actions.identifies<<','<<actions.equips<<','<<actions.refines<<','<<actions.reforges<<','<<settled<<','<<profile.daoYun<<','<<bossEntered<<','<<bossPhase<<','<<bossHp<<','<<x.progression.equipped.size()<<','<<immortal<<','<<dao<<','<<s.GetProgression().PityWaves()<<",\""<<build(x.progression.build)<<"\"\n"<<std::flush;
}
int main(int argc,char**argv) {
  try {
    std::ofstream trace(argc>1?argv[1]:"/tmp/dywx-full-natural-trace.csv");
    if(!trace)throw std::runtime_error("trace open failed");
    trace<<"sample,seed,mode,hero,cave,elapsed,wave,phase,hp,level,kills,stones,enemies\n";
    std::cout<<"seed,mode,hero,cave,profile,policy,bound_seconds,outcome,elapsed,wave,hp,kills,level,peak,opportunities,buys,reveals,identifies,equips,refines,reforges,settled,dao_yun,boss_entered,boss_phase,last_boss_hp,equipped,immortal_equipped,dao_equipped,pity_waves,build\n";
    const unsigned seeds[]{11,29,73};unsigned ordinal=0;
    for(int mode=0;mode<3;++mode)for(int hero=0;hero<3;++hero)
      run(seeds[hero],HeroId(hero),CaveId((hero+mode)%3),GameMode(mode),++ordinal,trace);
    run(11,HeroId::Body,CaveId::ThunderMarsh,GameMode::Quick,++ordinal,trace,true);
  }catch(const std::exception& e){std::cerr<<"natural-smoke-error: "<<e.what()<<'\n';return 1;}
}
