#include "MVP/Simulation.h"
#include "MVP/SaveCodec.h"
#include <cassert>
#include <iostream>
using namespace dywx::mvp;

static RunCheckpoint Checkpoint(ProfileStore& store, bool oldWaveReset = false) {
  const auto id=store.AllocateRunId();assert(id);
  Simulation run;RunConfig config;config.spawning=false;config.automaticSpells=false;
  CampaignConfig campaign;campaign.traditionalWaveSeconds=1;
  assert(run.StartCampaign(store.Data().campaign,GameMode::Traditional,HeroId::Body,11,id,
                           CaveId::ThunderMarsh,0,config,campaign));
  run.GetProgression().SetPityWaves(19);run.RefreshProgression();
  for(int i=0;i<20&&run.GetSnapshot().phase==RunPhase::Wave;++i)run.Step(.1f);
  assert(run.GetSnapshot().phase==RunPhase::Rest);
  auto state=run.GetCampaign().GetState();
  for(unsigned i=0;i<state.offerCount;++i)if(state.offers[i]==OpportunityKind::Event){assert(run.ChooseOpportunity(i));break;}
  RunCheckpoint checkpoint;assert(run.PrepareCheckpoint(checkpoint));
  if(oldWaveReset) {
    // Recreate the serialized flag from an older version, after projecting the
    // boundary (new EndRest correctly clears it). No schema migration required.
    auto relic=checkpoint.progression.OnKill(EnemyTier::Elite);
    assert(relic&&relic->rarity==Rarity::Immortal);
  }
  std::string bytes;assert(SaveCodec::EncodeCheckpoint(checkpoint,bytes));
  assert(store.SaveCheckpoint(HeroId::Body,bytes));
  RunCheckpoint decoded;assert(SaveCodec::DecodeCheckpoint(bytes,decoded));return decoded;
}
static void ObservePity(ProfileStore& store,int count) {
  Progression account;account.Reset(2,HeroId::Sword);account.SetPityWaves(count);
  store.Observe(account.GetState());assert(store.Save());
}
static void AccountRestore() {
  bool fail=false;std::string durable;
  ProfileStore store([&](const std::string& bytes){if(fail)return false;durable=bytes;return true;});
  const auto checkpoint=Checkpoint(store);
  // A suspended at 20. B consumes the account guarantee, which must win over A.
  Progression b;b.Reset(77,HeroId::Sword);b.SetPityWaves(20);
  auto immortal=b.OnKill(EnemyTier::Elite);assert(immortal&&immortal->rarity==Rarity::Immortal);
  store.Observe(b.GetState());assert(store.Save());
  Simulation live;live.Start(99,HeroId::Sword);const auto liveHero=live.GetSnapshot().progression.build.hero;
  Simulation candidate;assert(candidate.RestoreCheckpoint(checkpoint,store.Data().campaign));
  const auto oldXp=candidate.GetProgression().GetState().xp;
  // Same production method used by FDywxMVPProfile::Apply in ResumeRun.
  candidate.GetProgression().ImportAccountProgress(store.Data().known);candidate.RefreshProgression();
  assert(candidate.GetProgression().PityWaves()==0);
  assert(candidate.GetProgression().DropRarityProbabilities()[2]<1);
  assert(candidate.GetProgression().GetState().xp==oldXp);
  assert(candidate.GetProgression().GetState().seenSpells[int(SpellId::FlameSword)]);
  assert(candidate.GetProgression().GetState().seenSpells[int(SpellId::FlamePalm)]);
  const auto before=durable;std::string out="unchanged";fail=true;
  assert(!store.ConsumeCheckpoint(HeroId::Body,out));
  assert(durable==before&&out=="unchanged"&&store.HasCheckpoint(HeroId::Body));
  assert(store.Data().known.pityWaves==0&&live.GetSnapshot().progression.build.hero==liveHero);
  fail=false;assert(store.ConsumeCheckpoint(HeroId::Body,out));live=std::move(candidate);
  assert(live.GetProgression().PityWaves()==0);
  assert(!store.HasCheckpoint(HeroId::Body));
  // Growth also imports exactly: this must not retain the old smaller count.
  ObservePity(store,23);assert(candidate.RestoreCheckpoint(checkpoint,store.Data().campaign));
  candidate.GetProgression().ImportAccountProgress(store.Data().known);candidate.RefreshProgression();
  assert(candidate.GetProgression().PityWaves()==23);
  auto guaranteed=candidate.GetProgression().OnKill(EnemyTier::Elite);
  assert(guaranteed&&guaranteed->rarity==Rarity::Immortal);
}
static void OldCheckpointFlag() {
  ProfileStore store([](const std::string&){return true;});
  auto old=Checkpoint(store,true);assert(old.progression.PityWaves()==0);
  ObservePity(store,7);Simulation restored;assert(restored.RestoreCheckpoint(old,store.Data().campaign));
  restored.GetProgression().ImportAccountProgress(store.Data().known);
  restored.GetProgression().OnWaveEnded();assert(restored.GetProgression().PityWaves()==8);
}
static void CheckpointAccountTransaction() {
  bool fail=false;int writes=0;std::string durable;
  ProfileStore store([&](const std::string& bytes){++writes;if(fail)return false;durable=bytes;return true;});
  auto checkpoint=Checkpoint(store);ObservePity(store,20);
  std::string oldSlot;assert(store.GetCheckpoint(HeroId::Body,oldSlot));
  // A wave-end save must include a just-opened immortal chest's reset even if
  // the later StopRun profile save fails. Discovery flags share this transaction.
  checkpoint.progression.SetPityWaves(0);
  auto latest=checkpoint.progression.GetState();latest.seenRelics.back()=true;
  std::string updated;assert(SaveCodec::EncodeCheckpoint(checkpoint,updated));
  auto candidate=store;candidate.Observe(latest);
  candidate.MutableCampaign().seenAnomalies[2]=true;
  candidate.MutableCampaign().seenBosses[1]=true;
  const auto before=durable;fail=true;
  assert(!candidate.SaveCheckpoint(HeroId::Body,updated));
  assert(store.Data().known.pityWaves==20&&!store.Data().known.seenRelics.back());
  assert(!store.Data().campaign.seenAnomalies[2]&&!store.Data().campaign.seenBosses[1]);
  std::string unchanged;assert(store.GetCheckpoint(HeroId::Body,unchanged));
  assert(unchanged==oldSlot&&durable==before);
  fail=false;const int previousWrites=writes;
  assert(candidate.SaveCheckpoint(HeroId::Body,updated));assert(writes==previousWrites+1);
  store=std::move(candidate);
  assert(store.Data().known.pityWaves==0&&store.Data().known.seenRelics.back());
  assert(store.Data().campaign.seenAnomalies[2]&&store.Data().campaign.seenBosses[1]);
  // Simulate a subsequent failed profile write; the checkpoint and authoritative
  // account reset already coexist durably, independent of this later write.
  auto stopCandidate=store;stopCandidate.MutableCampaign().seenAnomalies[3]=true;
  const auto committed=durable;fail=true;assert(!stopCandidate.Save());assert(durable==committed);
  ProfileStore reloaded;assert(reloaded.Load(durable));
  assert(reloaded.Data().known.pityWaves==0);
  assert(reloaded.GetCheckpoint(HeroId::Body,updated));RunCheckpoint decoded;
  assert(SaveCodec::DecodeCheckpoint(updated,decoded));
  decoded.progression.ImportAccountProgress(reloaded.Data().known);
  assert(decoded.progression.PityWaves()==0);
  assert(decoded.progression.DropRarityProbabilities()[2]<1);
}
static void Drain(Simulation& s) {
  for(int i=0;s.GetSnapshot().progression.pendingUpgrades>0;++i){assert(i<100);assert(s.ChooseUpgrade(0));}
}
static void FinishWave(Simulation& s) {
  for(int i=0;i<20&&s.GetSnapshot().phase==RunPhase::Wave;++i)s.Step(.1f);
  assert(s.GetSnapshot().phase==RunPhase::Rest);
}
static void ExtraCombatBoundaries() {
  CampaignProfile profile;profile.anyWin=profile.traditionalWin=true;
  RunConfig config;config.spawning=false;config.automaticSpells=false;config.bossHp1=config.bossHp2=2;
  CampaignConfig campaign;campaign.traditionalWaveSeconds=campaign.endlessWaveSeconds=1;
  Simulation elite;assert(elite.StartCampaign(profile,GameMode::Traditional,HeroId::Body,1,1,
      CaveId::ThunderMarsh,0,config,campaign));FinishWave(elite);
  const auto offered=elite.GetCampaign().GetState();bool selected=false;
  for(unsigned i=0;i<offered.offerCount;++i)if(offered.offers[i]==OpportunityKind::Elite){assert(elite.ChooseOpportunity(i));selected=true;break;}
  assert(selected);Drain(elite);assert(elite.BeginNextWave());
  elite.GetProgression().SetPityWaves(20);elite.RefreshProgression();
  for(int i=0;!elite.GetSnapshot().enemies.empty();++i){assert(i<50);elite.TestDamageEnemy(elite.GetSnapshot().enemies.front().id,1e9f);}
  assert(elite.GetSnapshot().phase==RunPhase::Rest&&elite.GetProgression().PityWaves()==0);
  Drain(elite);assert(elite.BeginNextWave());FinishWave(elite);assert(elite.GetProgression().PityWaves()==1);

  Simulation endless;assert(endless.StartCampaign(profile,GameMode::Endless,HeroId::Body,2,2,
      CaveId::ThunderMarsh,0,config,campaign));
  for(int wave=1;wave<=5;++wave){
    FinishWave(endless);const auto offers=endless.GetCampaign().GetState();bool chosen=false;
    for(unsigned i=0;i<offers.offerCount;++i)if(offers.offers[i]==OpportunityKind::Cave||offers.offers[i]==OpportunityKind::Event){assert(endless.ChooseOpportunity(i));chosen=true;break;}
    assert(chosen);Drain(endless);assert(endless.BeginNextWave());
  }
  assert(endless.GetSnapshot().phase==RunPhase::Boss);
  // Boss rewards themselves are excluded from pity. A random immortal from a
  // boss-phase minion still resets it and must not poison the following wave.
  endless.GetProgression().SetPityWaves(20);endless.RefreshProgression();
  const auto minion=endless.TestSpawn(EnemyRole::Swarm,{0,0},EnemyTier::Elite);
  endless.TestDamageEnemy(minion,1e9f);assert(endless.GetProgression().PityWaves()==0);
  EntityId boss=0;for(const auto& e:endless.GetSnapshot().enemies)if(e.role==EnemyRole::Boss)boss=e.id;
  assert(boss);endless.TestDamageEnemy(boss,1e9f);endless.TestDamageEnemy(boss,1e9f);
  assert(endless.GetSnapshot().phase==RunPhase::Rest);assert(endless.ChooseBossReward(0));
  Drain(endless);assert(endless.BeginNextWave());assert(endless.GetSnapshot().wave==6);
  FinishWave(endless);assert(endless.GetProgression().PityWaves()==1);
}
static void RestChest() {
  Progression p;p.Reset(1,HeroId::Body);Economy e;e.Reset(2,false,true,true);
  p.SetPityWaves(20);p.BeginRest();assert(e.BeginRest(p,1,false));
  assert(e.OpenTreasure(p,false,true));assert(e.RevealTreasure(p));assert(p.PityWaves()==0);
  e.EndRest(p);p.EndRest();p.OnWaveEnded();assert(p.PityWaves()==1);
  // Combat drops still suppress that same wave's count, not the subsequent one.
  p.SetPityWaves(20);assert(p.OnKill(EnemyTier::Elite));p.OnWaveEnded();assert(p.PityWaves()==0);
  p.OnWaveEnded();assert(p.PityWaves()==1);
}
int main(){AccountRestore();OldCheckpointFlag();CheckpointAccountTransaction();ExtraCombatBoundaries();RestChest();std::cout<<"PASS account pity: authoritative resets/growth, discovery union, old wave flag, consume/save failure, atomic discoveries, chest/elite/endless boss boundaries\n";}
