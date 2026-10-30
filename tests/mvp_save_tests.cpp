#include "MVP/SaveCodec.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
static RunCheckpoint Fixture(){
  RunCheckpoint c;c.seed=345;c.simulationRng=789;c.nextEntityId=89;c.restSerial=1;
  CampaignProfile profile;profile.anyWin=profile.traditionalWin=true;Campaign campaign;
  assert(campaign.Start(profile,GameMode::Traditional,0,c.seed,1));assert(campaign.CompleteWave());
  for(unsigned i=0;i<campaign.GetState().offerCount;++i)if(campaign.GetState().offers[i]!=OpportunityKind::Elite){assert(campaign.ChooseOpportunity(i).accepted);break;}
  assert(campaign.BeginNextWave());c.campaign=campaign.GetState();
  c.progression.Reset(124,HeroId::Body);c.progression.ConfigureFullGame(3);c.economy.Reset(567,false,true,true);
  c.economy.ReceiveRelic(c.progression,c.progression.GenerateRelic(Rarity::Immortal));
  c.progression.BeginRest();assert(c.economy.BeginRest(c.progression,1,true,true));assert(c.economy.CreditStones(1000));
  assert(c.economy.Buy(c.progression,c.economy.Shop()[0].token));c.economy.EndRest(c.progression);c.progression.EndRest();
  c.arena=GenerateArena(c.seed);c.anomalies.Reset(c.seed,c.arena,true);c.anomalies.Step(10,{0,0},false);
  c.combat.speedBuff=1.25f;c.spellCooldowns[0]=.5f;c.casts[0]=7;return c;
}
static void Put(std::string& s,std::size_t offset,std::uint64_t n){for(unsigned i=0;i<8;++i)s[offset+i]=char((n>>(i*8))&255);}
static void Rehash(std::string& s){std::uint64_t h=1469598103934665603ull;for(std::size_t i=40;i<s.size();++i){h^=static_cast<unsigned char>(s[i]);h*=1099511628211ull;}Put(s,24,h);}
int main(){
  auto c=Fixture();c.campaignConfig.quickSixWaves=true;std::string blob;assert(SaveCodec::EncodeCheckpoint(c,blob));RunCheckpoint restored;assert(SaveCodec::DecodeCheckpoint(blob,restored));
  assert(restored.campaignConfig.quickSixWaves);
  assert(restored.economy.Stones()==c.economy.Stones());assert(restored.economy.IsUnidentified(c.progression.GetState().equipped[0].id));
  assert(restored.combat.speedBuff==c.combat.speedBuff&&restored.casts==c.casts);
  std::string twice;assert(SaveCodec::EncodeCheckpoint(restored,twice));assert(blob==twice);
  for(int i=0;i<20;++i){auto a=c.progression.GenerateRelic(Rarity::Spirit),b=restored.progression.GenerateRelic(Rarity::Spirit);assert(a.id==b.id&&a.catalogId==b.catalogId&&a.affixes.size()==b.affixes.size());for(std::size_t j=0;j<a.affixes.size();++j)assert(a.affixes[j].value==b.affixes[j].value);}
  for(int i=0;i<80;++i){auto a=c.anomalies.Step(.5f,{0,0},false),b=restored.anomalies.Step(.5f,{0,0},false);assert(a.size()==b.size());assert(c.anomalies.State().id==restored.anomalies.State().id);assert(c.anomalies.State().remaining==restored.anomalies.State().remaining);}
  for(std::size_t n=0;n<blob.size();n+=13){RunCheckpoint unchanged=restored;assert(!SaveCodec::DecodeCheckpoint(blob.substr(0,n),unchanged));assert(unchanged.seed==restored.seed);}
  auto bad=blob;bad.back()^=1;assert(!SaveCodec::DecodeCheckpoint(bad,restored));
  bad=blob;Put(bad,40,99);Rehash(bad);assert(!SaveCodec::DecodeCheckpoint(bad,restored)); // illegal hero enum, valid checksum
  bad=blob;Put(bad,48,std::numeric_limits<std::uint64_t>::max());Rehash(bad);assert(!SaveCodec::DecodeCheckpoint(bad,restored)); // hostile vector length
  bad=blob;Put(bad,32,SaveCodec::MaxBytes+1);assert(!SaveCodec::DecodeCheckpoint(bad,restored));
  bad=blob;Put(bad,8,999);assert(!SaveCodec::DecodeCheckpoint(bad,restored));
  bad=blob+"extra";assert(!SaveCodec::DecodeCheckpoint(bad,restored));
  auto invalid=Fixture();invalid.player.hp=std::numeric_limits<float>::quiet_NaN();assert(!SaveCodec::EncodeCheckpoint(invalid,twice));
  invalid=Fixture();invalid.campaign.mode=GameMode::Quick;assert(!SaveCodec::EncodeCheckpoint(invalid,twice));
  invalid=Fixture();invalid.campaign.hero=2;assert(!SaveCodec::EncodeCheckpoint(invalid,twice));
  invalid=Fixture();invalid.progression.BeginRest();assert(!SaveCodec::EncodeCheckpoint(invalid,twice));
  std::string disk;bool fail=false;int writes=0;
  ProfileStore store([&](const std::string& bytes){++writes;if(fail)return false;disk=bytes;return true;});
  fail=true;assert(store.AllocateRunId()==0);assert(store.Data().campaign.nextRunId==1&&disk.empty());
  fail=false;assert(store.AllocateRunId()==1);assert(store.Data().campaign.nextRunId==2);assert(store.SaveCheckpoint(HeroId::Body,blob));assert(store.HasCheckpoint(HeroId::Body));assert(!store.HasCheckpoint(HeroId::Sword));
  assert(!store.SaveCheckpoint(HeroId::Sword,blob));std::string output="unchanged";std::string oldDisk=disk;
  fail=true;assert(!store.ConsumeCheckpoint(HeroId::Body,output));assert(output=="unchanged"&&disk==oldDisk&&store.HasCheckpoint(HeroId::Body));
  fail=false;assert(store.GetCheckpoint(HeroId::Body,output)&&output==blob);assert(store.ConsumeCheckpoint(HeroId::Body,output));assert(output==blob&&!store.HasCheckpoint(HeroId::Body));assert(!store.ConsumeCheckpoint(HeroId::Body,output));
  // Ending a saved run invalidates its old slot in the same commit; write failure retains it.
  assert(store.SaveCheckpoint(HeroId::Body,blob));
  auto settled=store;settled.MutableCampaign().settledThrough[0]=1;
  fail=true;assert(!settled.Save());assert(store.HasCheckpoint(HeroId::Body));
  fail=false;assert(settled.Save());assert(!settled.HasCheckpoint(HeroId::Body));
  ProfileStore reload;assert(reload.Load(disk));assert(reload.Data().campaign.nextRunId==2&&!reload.HasCheckpoint(HeroId::Body));
  auto before=reload.Data().campaign.nextRunId;assert(!reload.Load(disk.substr(0,disk.size()-1)));assert(reload.Data().campaign.nextRunId==before);
  ProfileData malformed=reload.Data();malformed.campaign.nextRunId=0;assert(!SaveCodec::EncodeProfile(malformed,twice));
  ProfileData discovery;discovery.campaign.nextRunId=8;discovery.campaign.settledThrough={7,6,0};
  discovery.campaign.seenBosses={false,true,false};discovery.campaign.seenAnomalies[3]=true;
  discovery.campaign.endlessRanking={{7,0,12,90},{6,1,11,200}};
  std::string rankedBlob;assert(SaveCodec::EncodeProfile(discovery,rankedBlob));ProfileData decoded;
  assert(SaveCodec::DecodeProfile(rankedBlob,decoded));assert(decoded.campaign.seenBosses==discovery.campaign.seenBosses);
  assert(decoded.campaign.seenAnomalies[3]&&decoded.campaign.endlessRanking.size()==2);
  assert(decoded.campaign.endlessRanking[0].runId==7&&decoded.campaign.endlessRanking[1].hero==1&&decoded.campaign.endlessRanking[1].kills==200);
  auto rankingBad=discovery;std::swap(rankingBad.campaign.endlessRanking[0],rankingBad.campaign.endlessRanking[1]);assert(!SaveCodec::EncodeProfile(rankingBad,twice));
  rankingBad=discovery;rankingBad.campaign.endlessRanking[1].runId=7;assert(!SaveCodec::EncodeProfile(rankingBad,twice));
  rankingBad=discovery;rankingBad.campaign.endlessRanking[0].hero=3;assert(!SaveCodec::EncodeProfile(rankingBad,twice));
  rankingBad=discovery;rankingBad.campaign.endlessRanking.resize(21);assert(!SaveCodec::EncodeProfile(rankingBad,twice));
  ProgressionState legacy;legacy.seenSpells[0]=true;legacy.seenRelics[14]=true;legacy.pityWaves=17;assert(reload.ImportLegacy(legacy));assert(reload.Data().known.seenRelics[14]&&reload.Data().known.pityWaves==17);
  int priorWrites=writes;store.MutableCampaign();assert(store.Save());assert(writes==priorWrites);
  assert(writes>=5);std::cout<<"Save codec and profile transaction tests passed\n";
}
