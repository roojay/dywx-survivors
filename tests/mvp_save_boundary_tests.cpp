#include "MVP/SaveCodec.h"
#include <cassert>
#include <iostream>
using namespace dywx::mvp;
int main() {
  RunCheckpoint c; c.restSerial=1;
  CampaignProfile profile; profile.anyWin=profile.traditionalWin=true;
  Campaign campaign; assert(campaign.Start(profile,GameMode::Traditional,0,1,1));
  assert(campaign.CompleteWave());
  for(unsigned i=0;i<campaign.GetState().offerCount;++i)
    if(campaign.GetState().offers[i]!=OpportunityKind::Elite){assert(campaign.ChooseOpportunity(i).accepted);break;}
  assert(campaign.BeginNextWave()); c.campaign=campaign.GetState();
  c.progression.Reset(41,HeroId::Body); c.progression.ConfigureFullGame(3);
  c.economy.Reset(42,false,true,true); c.progression.BeginRest();
  assert(c.economy.BeginRest(c.progression,1,false));
  c.economy.EndRest(c.progression); c.progression.EndRest();
  c.arena=GenerateArena(c.seed); c.anomalies.Reset(c.seed,c.arena,true);
  std::string valid; assert(SaveCodec::EncodeCheckpoint(c,valid));
  c.restSerial=0; std::string bad;
  if(SaveCodec::EncodeCheckpoint(c,bad)) {
    RunCheckpoint restored; assert(SaveCodec::DecodeCheckpoint(bad,restored));
    restored.progression.BeginRest();
    assert(!restored.economy.BeginRest(restored.progression,++restored.restSerial,false));
    std::cout << "BUG: inconsistent rest serial accepted; next intermission economy cannot open\n";
    return 1;
  }
  RunCheckpoint restored; assert(SaveCodec::DecodeCheckpoint(valid,restored));
  restored.progression.BeginRest();
  assert(restored.economy.BeginRest(restored.progression,++restored.restSerial,false));
  std::cout << "Save boundary tests passed\n";
}
