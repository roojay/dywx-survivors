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

#include <fstream>
int main(int argc,char** argv){
 if(argc!=2)return 10;
 auto c=Fixture();
 c.runConfig.spawning=false;c.campaignConfig.traditionalWaveSeconds=4;
 c.progression.AddXp(23);
 std::string checkpoint,bytes;
 if(!SaveCodec::EncodeCheckpoint(c,checkpoint))return 11;
 ProfileData p;p.campaign.anyWin=p.campaign.traditionalWin=true;p.campaign.nextRunId=2;
 p.checkpoints[0]=checkpoint;
 if(!SaveCodec::EncodeProfile(p,bytes))return 12;
 std::ofstream f(argv[1],std::ios::binary);f.write(bytes.data(),bytes.size());
 std::cout<<"UI fixture only: no spawns, 4s waves, pending upgrade, wave 2, bytes="<<bytes.size()<<"\n";
 return f?0:13;
}
