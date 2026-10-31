#include "MVP/Simulation.h"
#include "MVP/SaveCodec.h"
#include <cassert>
#include <iostream>
#include <sstream>
using namespace dywx::mvp;
static RunConfig Fast() { RunConfig c;c.spawning=false;c.automaticSpells=false;c.waveDurations.fill(.1f);c.bossHp1=c.bossHp2=1;return c; }
static CampaignConfig FastCampaign() { CampaignConfig c;c.traditionalWaveSeconds=c.endlessWaveSeconds=.1f;return c; }
static void Rest(Simulation& s) { for(int i=0;i<20 && s.GetSnapshot().phase==RunPhase::Wave;++i)s.Step(.1f);assert(s.GetSnapshot().phase==RunPhase::Rest); }
static void DrainUpgrades(Simulation& s) {
  for(int limit=0;s.GetSnapshot().progression.pendingUpgrades>0;++limit){assert(limit<100);assert(s.ChooseUpgrade(0));}
}
static void ResolveOpportunity(Simulation& s) {
  auto c=s.GetCampaign().GetState();
  if(!c.opportunityResolved) {
    unsigned index=0;while(index<c.offerCount && (c.offers[index]==OpportunityKind::Elite || c.offers[index]==OpportunityKind::Chest))++index;
    assert(index<c.offerCount);assert(s.ChooseOpportunity(index));assert(!s.ChooseOpportunity(index));
  }
  DrainUpgrades(s); // Explicitly resolve event upgrades before the next-wave command.
}
static void Boss(Simulation& s) {
  assert(s.GetSnapshot().phase==RunPhase::Boss);
  EntityId boss=0;for(auto e:s.GetSnapshot().enemies)if(e.role==EnemyRole::Boss)boss=e.id;
  assert(boss);s.TestDamageEnemy(boss,1e9f);assert(s.GetSnapshot().bossPhase==2);
  s.TestDamageEnemy(boss,1e9f);assert(s.GetSnapshot().enemies.empty());
  assert(s.GetSnapshot().progression.bossRewards.size()==3);
  assert(s.ChooseBossReward(0));assert(!s.ChooseBossReward(0));
  if(s.GetSnapshot().phase==RunPhase::Rest)DrainUpgrades(s);
}
static std::string Fingerprint(const Simulation& s) {
  std::ostringstream o;const auto& x=s.GetSnapshot();const auto& p=x.progression;
  o<<int(x.phase)<<':'<<x.paused<<':'<<x.wave<<':'<<x.tick<<':'<<x.waveElapsed<<':'<<x.player.hp<<':'<<p.xp<<':'<<p.level<<':'<<p.pendingUpgrades<<':'<<s.GetEconomy().Stones()<<':'<<s.GetEconomy().PendingXp();
  for(auto offer:p.offers)o<<'/'<<int(offer.kind)<<','<<int(offer.spell)<<','<<int(offer.passive);
  for(const auto* list:{&p.equipped,&p.bag})for(const auto& item:*list){o<<'|'<<item.id<<':'<<item.catalogId;for(auto a:item.affixes)o<<','<<int(a.kind)<<'='<<a.value;}
  for(auto cd:x.spellCooldowns)o<<','<<cd;
  return o.str();
}
static void BossRuleDescriptions() {
  const auto thunder=Simulation::DescribeBossMechanics(CaveId::ThunderMarsh);
  assert(thunder.find("每15秒将全场其他妖魔的护盾重置为40（不叠加）")!=std::string::npos);
  assert(thunder.find("入场携2护持")!=std::string::npos);
  assert(thunder.find("破相时召4远程与6冲锋")!=std::string::npos);
  const auto ember=Simulation::DescribeBossMechanics(CaveId::EmberMountain);
  assert(ember.find("入场携2自爆")!=std::string::npos);
  assert(ember.find("破相时召4自爆与2远程")!=std::string::npos);
  std::ostringstream ring;ring<<"身周"<<Simulation::BossRules().ringRadius<<"格铺八处同等火域";
  assert(ember.find(ring.str())!=std::string::npos);
  const auto mist=Simulation::DescribeBossMechanics(CaveId::MistForest);
  assert(mist.find("入场携1护持与1召唤")!=std::string::npos);
  assert(mist.find("破相时召1护持、1召唤与4群行")!=std::string::npos);

  // Verify the disclosed entry/transition entourage against actual boss creation.
  // This deliberately shortened harness is a rule check, not balance evidence.
  CampaignProfile profile;
  profile.unlocks[unsigned(LegacyUnlock::RealmSecond)]=true;
  profile.unlocks[unsigned(LegacyUnlock::RealmThird)]=true;
  for(auto cave:{CaveId::ThunderMarsh,CaveId::EmberMountain,CaveId::MistForest}) {
    Simulation sim;assert(sim.StartCampaign(profile,GameMode::Quick,HeroId::Body,93,1,cave,0,Fast()));
    for(int wave=0;wave<4;++wave){Rest(sim);assert(sim.BeginNextWave());}
    std::array<int,8> entry{},added{};EntityId boss=0;
    for(const auto& enemy:sim.GetSnapshot().enemies){++entry[int(enemy.role)];if(enemy.role==EnemyRole::Boss)boss=enemy.id;}
    assert(boss);
    assert(entry[int(EnemyRole::Support)]==(cave==CaveId::ThunderMarsh?2:cave==CaveId::MistForest?1:0));
    assert(entry[int(EnemyRole::Exploder)]==(cave==CaveId::EmberMountain?2:0));
    assert(entry[int(EnemyRole::Summoner)]==(cave==CaveId::MistForest?1:0));
    if(cave==CaveId::ThunderMarsh) {
      std::vector<EntityId> supports;
      for(const auto& enemy:sim.GetSnapshot().enemies)if(enemy.role==EnemyRole::Support)supports.push_back(enemy.id);
      for(auto id:supports)sim.TestDamageEnemy(id,1e9f);
      const auto witness=sim.TestSpawn(EnemyRole::Swarm,{-30,-10});
      auto shield=[&](){for(const auto& enemy:sim.GetSnapshot().enemies)if(enemy.id==witness)return enemy.shield;return -1.f;};
      for(int tick=0;tick<140;++tick){sim.TestSetPlayer({40,20},120);sim.Step(.1f);}
      assert(shield()==0);
      for(int tick=0;tick<20;++tick){sim.TestSetPlayer({40,20},120);sim.Step(.1f);}
      assert(shield()==40);
      // The 15-second rule must not refill the boss itself.
      for(const auto& enemy:sim.GetSnapshot().enemies)if(enemy.id==boss)assert(enemy.shield==0);
      continue;
    }
    sim.TestDamageEnemy(boss,1e9f);
    for(const auto& enemy:sim.GetSnapshot().enemies)++added[int(enemy.role)];
    for(std::size_t i=0;i<entry.size();++i)added[i]-=entry[i];
    assert(added[int(EnemyRole::Exploder)]==(cave==CaveId::EmberMountain?4:0));
    assert(added[int(EnemyRole::Ranged)]==(cave==CaveId::EmberMountain?2:0));
    assert(added[int(EnemyRole::Support)]==(cave==CaveId::MistForest?1:0));
    assert(added[int(EnemyRole::Summoner)]==(cave==CaveId::MistForest?1:0));
    assert(added[int(EnemyRole::Swarm)]==(cave==CaveId::MistForest?4:0));
  }
  // Independent fresh Thunder encounter: no 16-second summons or shield fixture
  // may obscure the exact phase-transition entourage advertised above.
  Simulation thunderTransition;
  assert(thunderTransition.StartCampaign(profile,GameMode::Quick,HeroId::Body,94,2,CaveId::ThunderMarsh,0,Fast()));
  for(int wave=0;wave<4;++wave){Rest(thunderTransition);assert(thunderTransition.BeginNextWave());}
  std::array<int,8> before{},after{};EntityId thunderBoss=0;
  for(const auto& enemy:thunderTransition.GetSnapshot().enemies) {
    ++before[int(enemy.role)];if(enemy.role==EnemyRole::Boss)thunderBoss=enemy.id;
  }
  assert(thunderBoss);thunderTransition.TestDamageEnemy(thunderBoss,1e9f);
  assert(thunderTransition.GetSnapshot().bossPhase==2);
  for(const auto& enemy:thunderTransition.GetSnapshot().enemies)++after[int(enemy.role)];
  for(std::size_t role=0;role<before.size();++role) {
    const int expected=role==unsigned(EnemyRole::Ranged)?4:role==unsigned(EnemyRole::Charger)?6:0;
    assert(after[role]-before[role]==expected);
  }
}
static void ShopDiscovery() {
  CampaignProfile profile;profile.anyWin=true;
  Simulation quick;
  assert(quick.StartCampaign(profile,GameMode::Quick,HeroId::Body,103,1,CaveId::ThunderMarsh,0,Fast()));
  assert(!quick.DiscoverCurrentShop()); // Combat cannot manufacture presentation discoveries.
  Rest(quick);
  const auto stock=quick.GetEconomy().ExportState();
  auto expectedProgression=quick.GetProgression();
  const auto before=Fingerprint(quick);
  for(const auto& good:stock.shop)assert(!quick.GetSnapshot().progression.seenRelics[good.relic.catalogId]);
  quick.Pause(true);assert(!quick.DiscoverCurrentShop());quick.Pause(false);
  assert(quick.DiscoverCurrentShop());assert(quick.DiscoverCurrentShop());
  assert(Fingerprint(quick)==before);
  const auto after=quick.GetEconomy().ExportState();
  assert(after.rng==stock.rng && after.token==stock.token && after.stones==stock.stones);
  assert(after.shop.size()==stock.shop.size());
  for(std::size_t i=0;i<stock.shop.size();++i) {
    const auto& good=after.shop[i];const auto& old=stock.shop[i];
    assert(good.token==old.token && good.price==old.price && good.sold==old.sold);
    assert(good.relic.id==old.relic.id && good.relic.catalogId==old.relic.catalogId);
    assert(good.relic.affixes.size()==old.relic.affixes.size());
    for(std::size_t a=0;a<old.relic.affixes.size();++a) {
      const auto& x=good.relic.affixes[a];const auto& y=old.relic.affixes[a];
      assert(x.kind==y.kind && x.value==y.value && x.code==y.code);
    }
    assert(quick.GetSnapshot().progression.seenRelics[good.relic.catalogId]);
  }
  // Existing account observation/persistence, including no-purchase discoveries.
  std::string bytes;ProfileStore store([&](const std::string& value){bytes=value;return true;});
  store.Observe(quick.GetSnapshot().progression);assert(store.Save());
  ProfileStore loaded;assert(loaded.Load(bytes));
  assert(loaded.Data().known.seenRelics==quick.GetSnapshot().progression.seenRelics);
  const auto expectedRoll=expectedProgression.GenerateRelic(Rarity::Spirit);
  const auto actualRoll=quick.GetProgression().GenerateRelic(Rarity::Spirit);
  assert(actualRoll.id==expectedRoll.id && actualRoll.catalogId==expectedRoll.catalogId);
  assert(actualRoll.affixes.size()==expectedRoll.affixes.size());
  for(std::size_t i=0;i<actualRoll.affixes.size();++i) {
    const auto& x=actualRoll.affixes[i];const auto& y=expectedRoll.affixes[i];
    assert(x.kind==y.kind && x.value==y.value && x.code==y.code);
  }

  Simulation locked;assert(locked.StartCampaign({},GameMode::Quick,HeroId::Body,104,1,CaveId::ThunderMarsh,0,Fast()));
  Rest(locked);assert(!locked.DiscoverCurrentShop());
  Simulation traditional;
  assert(traditional.StartCampaign(profile,GameMode::Traditional,HeroId::Body,105,1,CaveId::ThunderMarsh,0,Fast(),FastCampaign()));
  Rest(traditional);assert(!traditional.DiscoverCurrentShop());
  const auto choices=traditional.GetCampaign().GetState();unsigned cave=0;
  while(cave<choices.offerCount && choices.offers[cave]!=OpportunityKind::Cave)++cave;
  assert(cave<choices.offerCount);assert(traditional.ChooseOpportunity(cave));
  for(const auto& good:traditional.GetEconomy().Shop())assert(!traditional.GetSnapshot().progression.seenRelics[good.relic.catalogId]);
  assert(traditional.DiscoverCurrentShop());
  assert(traditional.GetEconomy().CreditStones(1000));
  bool foundNew=false;
  for(int attempt=0;attempt<20 && !foundNew;++attempt) {
    const auto known=traditional.GetSnapshot().progression.seenRelics;
    assert(traditional.RefreshShop());
    for(const auto& good:traditional.GetEconomy().Shop()) {
      foundNew|=!known[good.relic.catalogId];
      assert(traditional.GetSnapshot().progression.seenRelics[good.relic.catalogId]==known[good.relic.catalogId]);
    }
    assert(traditional.DiscoverCurrentShop());
    for(const auto& good:traditional.GetEconomy().Shop())assert(traditional.GetSnapshot().progression.seenRelics[good.relic.catalogId]);
  }
  assert(foundNew);
  assert(traditional.BeginNextWave());assert(!traditional.DiscoverCurrentShop());
}
int main(){
  BossRuleDescriptions();
  ShopDiscovery();
  CampaignProfile p;
  Simulation s;
  assert(!s.StartCampaign(p,GameMode::Endless,HeroId::Body,5,1));
  assert(s.StartCampaign(p,GameMode::Quick,HeroId::Body,5,1,CaveId::ThunderMarsh,0,Fast(),FastCampaign()));
  assert(s.GetSnapshot().progression.systemLayer==1);
  assert(!s.GetAnomalies().State().enabled);
  auto enemy=s.TestSpawn(EnemyRole::Swarm,{0,0});s.TestDamageEnemy(enemy,1e9f);s.Step(.05f);
  assert(s.GetEconomy().Stones()==0);
  for(int wave=1;wave<=4;++wave){Rest(s);assert(s.GetEconomy().Shop().empty());assert(s.BeginNextWave());}
  Boss(s);assert(s.GetSnapshot().phase==RunPhase::Won);assert(s.SettleCampaign(p));assert(!s.SettleCampaign(p));assert(p.anyWin&&!p.traditionalWin);
  assert(s.StartCampaign(p,GameMode::Traditional,HeroId::Body,9,2,CaveId::ThunderMarsh,0,Fast(),FastCampaign()));
  assert(s.GetAnomalies().State().enabled);
  enemy=s.TestSpawn(EnemyRole::Swarm,{0,0});s.TestDamageEnemy(enemy,1e9f);s.Step(.05f);assert(s.GetEconomy().Stones()>0);
  for(int wave=1;wave<=10;++wave){Rest(s);assert(!s.BeginNextWave());ResolveOpportunity(s);auto before=s.GetSnapshot().statistics.elapsed;s.Step(1);assert(s.GetSnapshot().statistics.elapsed==before);assert(s.BeginNextWave());}
  Boss(s);assert(s.GetEconomy().Stones()==0);assert(s.SettleCampaign(p));assert(p.traditionalWin);
  assert(s.StartCampaign(p,GameMode::Endless,HeroId::Body,10,3,CaveId::ThunderMarsh,0,Fast(),FastCampaign()));
  for(int wave=1;wave<=10;++wave){
    Rest(s);ResolveOpportunity(s);assert(s.BeginNextWave());
    if(wave%5==0){Boss(s);assert(s.GetSnapshot().phase==RunPhase::Rest);assert(s.GetCampaign().GetState().bosses==wave/5);assert(s.BeginNextWave());}
  }
  assert(s.GetSnapshot().wave==11);s.TestDamagePlayer(1e9f);assert(s.GetSnapshot().phase==RunPhase::Dead);assert(s.SettleCampaign(p));assert(!s.SettleCampaign(p));
  // Layout used by player and enemy spawn, not an independent decorative map.
  assert(s.StartCampaign(p,GameMode::Quick,HeroId::Body,99,4,CaveId::ThunderMarsh,0,Fast(),FastCampaign()));
  assert(ValidateArena(s.GetArena()));
  for(auto obstacle:s.GetArena().obstacles){auto id=s.TestSpawn(EnemyRole::Swarm,obstacle.position);for(auto e:s.GetSnapshot().enemies)if(e.id==id)assert(s.GetArena().Walkable(e.position));}
  for(int i=0;i<20;++i){s.Step(.05f,{{1,1},false});}
  assert(s.GetArena().Walkable(s.GetSnapshot().player.position));
  // Every expanded spell has an executable combat path, not just a catalog row.
  for(int spell=12;spell<int(SpellId::Count);++spell) {
    Simulation combat;auto config=Fast();config.waveDurations.fill(100);
    assert(combat.StartCampaign(p,GameMode::Quick,HeroId::Body,700+spell,100+spell,CaveId::ThunderMarsh,0,config));
    combat.TestSpawn(EnemyRole::Boss,{1,0},EnemyTier::Boss);
    combat.TestCast(SpellId(spell),5);
    for(int tick=0;tick<100;++tick)combat.Step(.05f);
    assert(combat.GetSnapshot().statistics.damageDealt>0);
  }
  // Selected cave authorizes a shop; no unchosen or repeated opportunity can mint stock.
  Simulation cave;auto slow=Fast();slow.waveDurations.fill(100);
  assert(cave.StartCampaign(p,GameMode::Traditional,HeroId::Body,123,500,CaveId::ThunderMarsh,0,slow,FastCampaign()));
  Rest(cave);auto offers=cave.GetCampaign().GetState();unsigned caveIndex=0;
  while(caveIndex<offers.offerCount&&offers.offers[caveIndex]!=OpportunityKind::Cave)++caveIndex;
  assert(caveIndex<offers.offerCount);assert(cave.ChooseOpportunity(caveIndex));
  assert(cave.GetEconomy().Shop().size()>=3);
  assert(cave.GetEconomy().CreditStones(1000));
  auto token=cave.GetEconomy().Shop()[0].token;assert(cave.BuyGood(token));assert(!cave.BuyGood(token));
  auto relic=cave.GetProgression().GenerateRelic(Rarity::Immortal);auto relicId=relic.id;
  cave.GetEconomy().ReceiveRelic(cave.GetProgression(),relic);cave.RefreshProgression();
  assert(cave.GetEconomy().IsUnidentified(relicId));assert(cave.IdentifyRelic(relicId));assert(!cave.GetEconomy().IsUnidentified(relicId));
  assert(!cave.OpenTreasure(false));assert(cave.OpenTreasure(true));assert(!cave.BeginNextWave());assert(cave.RevealTreasure());
  assert(cave.BeginNextWave());assert(!cave.RefreshShop());
  // Elite opportunity is a real combat encounter and cannot be dismissed by the next-wave button.
  Simulation elite;assert(elite.StartCampaign(p,GameMode::Traditional,HeroId::Body,125,501,CaveId::ThunderMarsh,0,slow,FastCampaign()));
  Rest(elite);offers=elite.GetCampaign().GetState();unsigned eliteIndex=0;
  while(eliteIndex<offers.offerCount&&offers.offers[eliteIndex]!=OpportunityKind::Elite)++eliteIndex;
  if(eliteIndex<offers.offerCount){
    assert(elite.ChooseOpportunity(eliteIndex));
    assert(elite.GetSnapshot().phase==RunPhase::Rest);assert(elite.GetSnapshot().enemies.empty());
    assert(elite.BeginNextWave());assert(!elite.BeginNextWave());
    while(!elite.GetSnapshot().enemies.empty()){auto id=elite.GetSnapshot().enemies.front().id;elite.TestDamageEnemy(id,1e9f);}
    assert(elite.GetSnapshot().phase==RunPhase::Rest);DrainUpgrades(elite);assert(elite.BeginNextWave());
  }
  // Checkpoint stores the next-wave entrance, preserving build and RNG without replaying rewards.
  Simulation saved;assert(saved.StartCampaign(p,GameMode::Traditional,HeroId::Body,246,600,CaveId::ThunderMarsh,0,slow,FastCampaign()));
  Rest(saved);ResolveOpportunity(saved);RunCheckpoint checkpoint;
  saved.Pause(true);auto hpBefore=saved.GetSnapshot().player.hp;
  assert(saved.PrepareCheckpoint(checkpoint));assert(saved.GetSnapshot().paused);assert(saved.GetSnapshot().player.hp==hpBefore);std::string bytes;assert(SaveCodec::EncodeCheckpoint(checkpoint,bytes));
  RunCheckpoint decoded;assert(SaveCodec::DecodeCheckpoint(bytes,decoded));Simulation resumed;
  assert(resumed.RestoreCheckpoint(decoded,p));assert(!resumed.GetSnapshot().paused);saved.Pause(false);assert(saved.BeginNextWave());
  assert(resumed.GetSnapshot().wave==saved.GetSnapshot().wave);
  assert(resumed.GetEconomy().Stones()==saved.GetEconomy().Stones());
  assert(resumed.GetSnapshot().player.hp==saved.GetSnapshot().player.hp);
  assert(resumed.GetSnapshot().progression.build.spells.size()==saved.GetSnapshot().progression.build.spells.size());
  assert(resumed.GetArena().obstacles.size()==saved.GetArena().obstacles.size());
  Simulation pending;assert(pending.StartCampaign(p,GameMode::Traditional,HeroId::Body,247,601,CaveId::ThunderMarsh,0,slow,FastCampaign()));
  Rest(pending);ResolveOpportunity(pending);
  for(int i=0;i<7;++i)pending.GetEconomy().ReceiveRelic(pending.GetProgression(),pending.GetProgression().GenerateRelic(Rarity::Dao));
  pending.RefreshProgression();assert(!pending.GetSnapshot().progression.bag.empty());
  assert(pending.Refine({pending.GetSnapshot().progression.bag.back().id},RefineReward::Experience));
  pending.Pause(true);auto original=Fingerprint(pending);RunCheckpoint pendingCheckpoint;
  assert(pending.PrepareCheckpoint(pendingCheckpoint));assert(Fingerprint(pending)==original);
  assert(pendingCheckpoint.progression.GetState().pendingUpgrades>0);
  std::string first,second;assert(SaveCodec::EncodeCheckpoint(pendingCheckpoint,first));
  RunCheckpoint repeat;assert(pending.PrepareCheckpoint(repeat));assert(SaveCodec::EncodeCheckpoint(repeat,second));assert(first==second);
  assert(SaveCodec::DecodeCheckpoint(first,decoded));assert(resumed.RestoreCheckpoint(decoded,p));
  assert(resumed.GetSnapshot().progression.pendingUpgrades==pendingCheckpoint.progression.GetState().pendingUpgrades);
  assert(resumed.GetSnapshot().progression.offers.size()==pendingCheckpoint.progression.GetState().offers.size());
  auto restoreOffers=resumed.GetSnapshot().progression.offers;auto projectedOffers=pendingCheckpoint.progression.GetState().offers;
  for(std::size_t i=0;i<restoreOffers.size();++i){assert(restoreOffers[i].kind==projectedOffers[i].kind);assert(restoreOffers[i].spell==projectedOffers[i].spell);assert(restoreOffers[i].passive==projectedOffers[i].passive);}
  // Restore a different valid checkpoint again for stream comparison.
  assert(SaveCodec::DecodeCheckpoint(bytes,decoded));assert(resumed.RestoreCheckpoint(decoded,p));
  // The restored encounter uses the same spawn stream as an uninterrupted next wave.
  auto a=saved.TestSpawn(EnemyRole::Charger,{10,10},EnemyTier::Elite);
  auto b=resumed.TestSpawn(EnemyRole::Charger,{10,10},EnemyTier::Elite);
  assert(a==b);assert(saved.GetSnapshot().enemies.back().mutations==resumed.GetSnapshot().enemies.back().mutations);
  Simulation bossEntry;assert(bossEntry.StartCampaign(p,GameMode::Traditional,HeroId::Body,248,602,CaveId::ThunderMarsh,0,slow,FastCampaign()));
  for(int wave=1;wave<=10;++wave){Rest(bossEntry);ResolveOpportunity(bossEntry);if(wave<10)assert(bossEntry.BeginNextWave());}
  RunCheckpoint bossCheckpoint;assert(bossEntry.PrepareCheckpoint(bossCheckpoint));assert(bossCheckpoint.campaign.phase==CampaignPhase::Boss);
  assert(SaveCodec::EncodeCheckpoint(bossCheckpoint,first));assert(SaveCodec::DecodeCheckpoint(first,decoded));
  assert(resumed.RestoreCheckpoint(decoded,p));assert(bossEntry.BeginNextWave());
  assert(resumed.GetSnapshot().enemies.size()==bossEntry.GetSnapshot().enemies.size());
  for(std::size_t i=0;i<resumed.GetSnapshot().enemies.size();++i){auto a=resumed.GetSnapshot().enemies[i];auto b=bossEntry.GetSnapshot().enemies[i];assert(a.id==b.id&&a.hp==b.hp&&a.position.x==b.position.x&&a.position.y==b.position.y);}
  p.unlocks[unsigned(LegacyUnlock::RealmSecond)]=p.unlocks[unsigned(LegacyUnlock::RealmThird)]=true;
  std::array<std::string,3> eventTitles;
  for(int realm=0;realm<3;++realm){
    bool found=false;
    for(int seed=1;seed<30&&!found;++seed){
      Simulation event;assert(event.StartCampaign(p,GameMode::Traditional,HeroId::Body,seed,700+realm,CaveId(realm),0,slow,FastCampaign()));Rest(event);
      const auto& run=event.GetCampaign().GetState();
      for(unsigned i=0;i<run.offerCount;++i)if(run.offers[i]==OpportunityKind::Event){
        auto details=event.DescribeOpportunity(i);assert(!details.title.empty()&&!details.description.empty());
        assert(details.description==event.DescribeOpportunity(i).description);auto rng=run.rng;
        assert(event.GetCampaign().GetState().rng==rng);auto xp=event.GetSnapshot().progression.xp;
        assert(event.ChooseOpportunity(i));assert(event.GetSnapshot().progression.xp==xp+details.experience);
        assert(event.GetEconomy().Stones()==int(details.spiritStones));eventTitles[realm]=details.title;found=true;break;
      }
    }
    assert(found);
  }
  assert(eventTitles[0]!=eventTitles[1]&&eventTitles[1]!=eventTitles[2]);
  // All six anomalies run on combat time; tide affects real pickup income, not only tooltip text.
  Simulation weather;auto longCampaign=FastCampaign();longCampaign.traditionalWaveSeconds=1000;
  assert(weather.StartCampaign(p,GameMode::Traditional,HeroId::Body,999,800,CaveId::ThunderMarsh,0,slow,longCampaign));
  bool tideTested=false;
  for(int tick=0;tick<2200;++tick){
    DrainUpgrades(weather); // Explicit headless choices; never bypass the combat freeze.
    weather.Step(.1f);const auto& anomaly=weather.GetAnomalies().State();
    if(!tideTested&&anomaly.active&&anomaly.id==AnomalyId::SpiritTide&&anomaly.remaining>10){
      int before=weather.GetEconomy().Stones();
      for(int n=0;n<100;++n){DrainUpgrades(weather);auto id=weather.TestSpawn(EnemyRole::Swarm,{0,0});weather.TestDamageEnemy(id,1e9f);weather.Step(.02f);}
      int income=weather.GetEconomy().Stones()-before;assert(income>125&&income<175);tideTested=true;
    }
  }
  assert(tideTested);for(bool encountered:weather.GetAnomalies().State().encountered)assert(encountered);
  weather.Pause(true);auto weatherBefore=weather.GetAnomalies().State().elapsed;weather.Step(10);assert(weather.GetAnomalies().State().elapsed==weatherBefore);
  Simulation six;auto sixConfig=Fast();sixConfig.sixWaves=true;
  assert(six.StartCampaign(p,GameMode::Quick,HeroId::Body,50,900,CaveId::ThunderMarsh,0,sixConfig));assert(six.GetSnapshot().waveCount==6);
  for(int wave=1;wave<=6;++wave){Rest(six);assert(six.BeginNextWave());if(wave<6)assert(six.GetSnapshot().phase==RunPhase::Wave);}
  Boss(six);assert(six.GetSnapshot().phase==RunPhase::Won);
  // Each additional cave boss runs distinct warning-backed hazards in both transformations.
  for(CaveId realm:{CaveId::EmberMountain,CaveId::MistForest}){
    Simulation distinct;assert(distinct.StartCampaign(p,GameMode::Quick,HeroId::Body,51,901+int(realm),realm,0,Fast()));
    for(int wave=1;wave<=4;++wave){Rest(distinct);assert(distinct.BeginNextWave());}
    EntityId boss=0;for(auto enemy:distinct.GetSnapshot().enemies)if(enemy.role==EnemyRole::Boss)boss=enemy.id;
    assert(boss);assert(distinct.DescribeBossMechanics().find("初版")!=std::string::npos);
    for(int phase=1;phase<=2;++phase){
      bool sawWarning=false,sawRootSlow=false;std::vector<EntityId> remove;
      for(auto enemy:distinct.GetSnapshot().enemies)if(enemy.id!=boss)remove.push_back(enemy.id);
      for(auto id:remove)distinct.TestDamageEnemy(id,1e9f);
      distinct.TestSetPlayer({0,0},120);
      for(int tick=0;tick<85;++tick){
        distinct.Step(.1f);
        for(auto effect:distinct.GetSnapshot().effects)if(effect.owner==boss&&effect.hostile&&effect.damage>0){
          if(effect.warning>0)sawWarning=true;
          assert(effect.element==(realm==CaveId::EmberMountain?Element::Fire:Element::Wood));
          if(realm==CaveId::MistForest&&effect.warning<=0&&!sawRootSlow){
            distinct.TestSetPlayer(effect.position,120);auto before=distinct.GetSnapshot().player.position;
            distinct.Step(.1f,{{1,0},false});auto after=distinct.GetSnapshot().player.position;
            assert(after.x-before.x<.45f);sawRootSlow=true;
          }
        }
      }
      assert(sawWarning);if(realm==CaveId::MistForest)assert(sawRootSlow);
      if(phase==1)distinct.TestDamageEnemy(boss,1e9f);
    }
  }
  std::cout<<"full_game_integration_tests: campaign layers, 10-wave/endless bosses, settlement, economy and layout passed\n";
}
