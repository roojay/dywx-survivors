#include "MVP/Economy.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <locale>
namespace dywx::mvp {
namespace {
int Price(int base,int step,int count) {
  return int(std::min<std::int64_t>(std::numeric_limits<int>::max(), std::int64_t(base)+std::int64_t(step)*count));
}
bool Eligible(const Affix& a) { return a.kind != AffixKind::Mystery && a.kind != AffixKind::Unique; }
bool ValidReplacement(const Relic& relic,std::size_t index,const Affix& candidate) {
  if(index>=relic.affixes.size()||!Eligible(candidate))return false;
  unsigned triggers=candidate.kind==AffixKind::Trigger?1:0;
  for(std::size_t i=0;i<relic.affixes.size();++i)
    if(i!=index&&relic.affixes[i].kind==AffixKind::Trigger)++triggers;
  return triggers<=1;
}
}
void Economy::Reset(std::uint32_t seed,bool quick,bool l2,bool l3,EconomyConfig c) {
  *this=Economy{}; rng_.seed(seed); quick_=quick; layer2_=l2; layer3_=l2&&l3;
  for(auto& n:c.killStones)n=std::clamp(n,0,1000000);
  for(auto& n:c.refineStones)n=std::clamp(n,0,1000000);
  for(auto& n:c.shopPrices)n=std::clamp(n,1,1000000);
  c.refreshPrice=std::clamp(c.refreshPrice,1,1000000);
  c.reforgeBase=std::clamp(c.reforgeBase,1,1000000); c.reforgeStep=std::clamp(c.reforgeStep,1,1000000);
  c.treasureBase=std::clamp(c.treasureBase,1,1000000); c.treasureStep=std::clamp(c.treasureStep,1,1000000);
  c.paidTreasureCap=std::clamp(c.paidTreasureCap,0,5); c.traditionalGoods=std::clamp(c.traditionalGoods,3,5);
  c.pityWaves=std::clamp(c.pityWaves,1,1000000);
  for(auto& w:c.treasureWeights)if(!std::isfinite(w)||w<=0)w=.01;
  for(auto& w:c.treasureWeights)w=std::min(w,1000000.);
  c.rarityWeight=std::isfinite(c.rarityWeight)?std::clamp(c.rarityWeight,1.f,10.f):1.f;
  config_=c;
}
bool Economy::Rest(const Progression& p) const {return active_&&p.IsResting();}
bool Economy::BeginRest(Progression& p,std::uint64_t id,bool cave,bool freeRefresh) {
  if(!p.IsResting()||active_||id<=restId_||
     (layer2_&&(quick_||cave)&&token_>std::numeric_limits<std::uint64_t>::max()-unsigned(quick_?3:config_.traditionalGoods)))return false;
  active_=true;restId_=id;cave_=!quick_&&cave;freeRefresh_=freeRefresh&&cave_;shop_.clear();
  if(layer2_&&(quick_||cave_))GenerateShop(p);
  return true;
}
bool Economy::EnterCave(Progression& p,bool freeRefresh) {
  if(!Rest(p)||quick_||cave_||(layer2_&&token_>std::numeric_limits<std::uint64_t>::max()-unsigned(config_.traditionalGoods)))return false;
  cave_=true;freeRefresh_=freeRefresh;
  if(layer2_)GenerateShop(p);
  return true;
}
void Economy::EndRun() {
  stones_=0;pendingXp_=0;active_=false;cave_=false;freeRefresh_=false;
  shop_.clear();reforge_.reset();treasure_.reset();undo_.clear();
}
bool Economy::ChooseBossReward(Progression& p,std::size_t index) {
  if(!Rest(p)||index>=p.state_.bossRewards.size())return false;
  auto relic=p.state_.bossRewards[index];
  if(Find(p,relic.id))return false;
  p.state_.bossRewards.clear();
  ReceiveRelic(p,std::move(relic));return true;
}
void Economy::EndRest(Progression& p) {
  if(!active_)return;
  // Unresolved enchant keeps the old affix; purchased/revealed treasures are never lost.
  if(treasure_) { ReceiveRelic(p,*treasure_); treasure_.reset(); }
  reforge_.reset();undo_.clear();active_=false;shop_.clear();
  for(auto it=hidden_.begin();it!=hidden_.end();) {
    if(!Find(p,it->first))it=hidden_.erase(it);else ++it;
  }
  const int xp=pendingXp_; pendingXp_=0; p.AddRawXp(float(xp));
}
bool Economy::CommitRestExperience(Progression& p) {
  if(!Rest(p))return false;
  const int economyXp=pendingXp_,progressionXp=p.pendingRefineXp_;
  pendingXp_=0;p.pendingRefineXp_=0;p.state_.pendingRefineXp=0;
  undo_.erase(std::remove_if(undo_.begin(),undo_.end(),[](const EconomyUndo& u){return u.reward==RefineReward::Experience;}),undo_.end());
  p.undo_.clear();
  p.AddRawXp(float(economyXp));p.AddRawXp(float(progressionXp));
  return true;
}
bool Economy::CreditStones(int n) {
  if(!layer2_||n<=0||n>std::numeric_limits<int>::max()-stones_)return false;
  stones_+=n;return true;
}
void Economy::CreditKill(EnemyTier t) {const int i=int(t); if(i>=0&&i<4)CreditStones(config_.killStones[i]);}
void Economy::GenerateShop(Progression& p) {
  shop_.clear();int n=quick_?3:config_.traditionalGoods;
  for(int i=0;i<n;++i) {
    auto roll=rng_()%100;Rarity r=roll<5?Rarity::Immortal:(roll<60||i==0?Rarity::Spirit:Rarity::Common);
    shop_.push_back({++token_,p.GenerateRelic(r),config_.shopPrices[int(r)],false});
  }
}
bool Economy::Buy(Progression& p,std::uint64_t token) {
  if(!Rest(p)||!layer2_||(!quick_&&!cave_))return false;
  auto it=std::find_if(shop_.begin(),shop_.end(),[&](const auto& g){return g.token==token;});
  if(it==shop_.end()||it->sold||stones_<it->price||Find(p,it->relic.id))return false;
  stones_-=it->price;it->sold=true;ReceiveRelic(p,it->relic,true);return true;
}
bool Economy::Refresh(Progression& p) {
  if(!Rest(p)||!layer2_||quick_||!cave_||token_>std::numeric_limits<std::uint64_t>::max()-unsigned(config_.traditionalGoods))return false;
  if(freeRefresh_)freeRefresh_=false;
  else {if(stones_<config_.refreshPrice)return false;stones_-=config_.refreshPrice;}
  GenerateShop(p);return true;
}
EconomyRefinePreview Economy::PreviewRefine(const Progression& p,const std::vector<EntityId>& ids,RefineReward reward) const {
  EconomyRefinePreview out;out.reward=reward;
  if(!Rest(p)||(reward!=RefineReward::Experience&&reward!=RefineReward::Stones)||
     (reward==RefineReward::Stones&&!layer2_))return out;
  std::set<EntityId> seen;
  for(auto id:ids)for(const auto& r:p.GetState().bag)if(r.id==id&&!r.locked&&(!reforge_||reforge_->relic!=id)&&seen.insert(id).second) {
    if(int(r.rarity)<0||int(r.rarity)>3)continue;
    int value=reward==RefineReward::Stones?config_.refineStones[int(r.rarity)]:RefineValue(r.rarity);
    if(value>std::numeric_limits<int>::max()-out.amount)continue;
    out.ids.push_back(id);out.amount+=value;
  }
  return out;
}
bool Economy::Refine(Progression& p,const std::vector<EntityId>& ids,RefineReward reward) {
  auto preview=PreviewRefine(p,ids,reward);
  int& balance=reward==RefineReward::Stones?stones_:pendingXp_;
  if(preview.ids.empty()||preview.amount>std::numeric_limits<int>::max()-balance)return false;
  EconomyUndo u{{},preview.amount,reward};
  for(auto id:preview.ids) {
    auto it=std::find_if(p.state_.bag.begin(),p.state_.bag.end(),[&](const Relic& r){return r.id==id;});
    u.relics.push_back(*it);p.state_.bag.erase(it);
  }
  balance+=preview.amount;undo_.push_back(std::move(u));return true;
}
bool Economy::UndoRefine(Progression& p) {
  if(!Rest(p)||undo_.empty())return false;
  const auto& u=undo_.back();int& balance=u.reward==RefineReward::Stones?stones_:pendingXp_;
  if(balance<u.amount)return false;
  balance-=u.amount;p.state_.bag.insert(p.state_.bag.end(),u.relics.begin(),u.relics.end());undo_.pop_back();return true;
}
Relic* Economy::Find(Progression& p,EntityId id) {
  for(auto* list:{&p.state_.equipped,&p.state_.bag})for(auto& r:*list)if(r.id==id)return &r;
  return nullptr;
}
int Economy::ReforgePrice(EntityId id) const {
  auto it=reforges_.find(id);return Price(config_.reforgeBase,config_.reforgeStep,it==reforges_.end()?0:it->second);
}
bool Economy::BeginReforge(Progression& p,EntityId id,std::size_t index) {
  if(!Rest(p)||!layer3_||!cave_||reforge_||IsUnidentified(id))return false;
  auto* r=Find(p,id);int price=ReforgePrice(id);
  if(!r||r->locked||index>=r->affixes.size()||!Eligible(r->affixes[index])||stones_<price)return false;
  // Reuse authoritative catalog affix ranges at the same rarity; never roll Mystery/Unique.
  auto sample=p.GenerateRelic(r->rarity);std::vector<Affix> candidates;
  for(auto a:sample.affixes)if(ValidReplacement(*r,index,a)&&
    (a.kind!=r->affixes[index].kind||a.code!=r->affixes[index].code||a.value!=r->affixes[index].value))candidates.push_back(a);
  if(candidates.empty())return false;
  auto next=candidates[rng_()%candidates.size()];
  reforge_=ReforgeOffer{id,index,r->affixes[index],next,price};stones_-=price;
  if(reforges_[id]<std::numeric_limits<int>::max())++reforges_[id];
  return true;
}
bool Economy::ResolveReforge(Progression& p,bool keepNew) {
  if(!Rest(p)||!reforge_)return false;
  auto offer=*reforge_;auto* r=Find(p,offer.relic);
  if(keepNew&&(!r||r->locked||!ValidReplacement(*r,offer.index,offer.newAffix)))return false;
  if(keepNew)r->affixes[offer.index]=offer.newAffix;
  reforge_.reset();p.Recompute();return true;
}
std::array<double,4> Economy::TreasureProbabilities(const Progression& p) const {
  // Wave-counted shared pity, not number of paid openings. Dao is never guaranteed.
  if(p.PityWaves()>=config_.pityWaves)return {0,0,1,0};
  auto w=config_.treasureWeights;double luck=1+.015*EffectiveLuck(p.GetState().build.stats.luck);
  for(int i=1;i<4;++i)w[i]*=luck*config_.rarityWeight;
  w[2]*=1.+double(std::max(0,p.PityWaves()))/config_.pityWaves;
  double total=std::accumulate(w.begin(),w.end(),0.);for(auto& x:w)x/=total;return w;
}
int Economy::TreasurePrice() const {return Price(config_.treasureBase,config_.treasureStep,paidTreasure_);}
bool Economy::OpenTreasure(Progression& p,bool paid,bool encounter) {
  if(!Rest(p)||!layer3_||treasure_)return false;
  if(paid) {if(paidTreasure_>=config_.paidTreasureCap||stones_<TreasurePrice())return false;}
  else if(!encounter||freeTreasures_.count(restId_))return false;
  const auto probs=TreasureProbabilities(p);
  double roll=double(rng_())/(double(std::mt19937::max())+1.);int rarity=0;
  for(;rarity<3&&roll>=probs[rarity];++rarity)roll-=probs[rarity];
  if(paid){stones_-=TreasurePrice();++paidTreasure_;}else freeTreasures_.insert(restId_);
  treasure_=p.GenerateRelic(Rarity(rarity));
  if(rarity==2){p.SetPityWaves(0);p.immortalThisWave_=true;}
  return true;
}
std::optional<Relic> Economy::RevealTreasure(Progression& p) {
  if(!Rest(p)||!treasure_)return {};
  auto r=*treasure_;treasure_.reset();ReceiveRelic(p,r);
  const auto* granted=Find(p,r.id);return granted?std::optional<Relic>(*granted):std::nullopt;
}
void Economy::ReceiveRelic(Progression& p,Relic r,bool identified) {
  if(int(r.rarity)<0||int(r.rarity)>3||r.id==std::numeric_limits<EntityId>::max())return;
  if(r.id==0)r.id=p.nextRelic_++;
  if(Find(p,r.id))return;
  if(!quick_&&!identified&&r.rarity>=Rarity::Immortal) {hidden_[r.id]=r.affixes;r.affixes.clear();}
  p.GrantRelic(std::move(r));
}
bool Economy::Identify(Progression& p,EntityId id) {
  if(!Rest(p)||!cave_)return false;
  auto it=hidden_.find(id);auto* r=Find(p,id);if(it==hidden_.end()||!r)return false;
  r->affixes=it->second;
  // Identification may reveal an incompatible mystery: move it to bag, never activate a conflict.
  auto equipped=std::find_if(p.state_.equipped.begin(),p.state_.equipped.end(),[&](const Relic& x){return x.id==id;});
  if(equipped!=p.state_.equipped.end()&&!p.Compatible(*r,std::size_t(equipped-p.state_.equipped.begin()))) {
    p.state_.bag.push_back(*r);p.state_.equipped.erase(equipped);
  }
  hidden_.erase(it);p.Recompute();return true;
}
EconomyState Economy::ExportState() const {
  EconomyState s;std::ostringstream rng;rng.imbue(std::locale::classic());rng<<rng_;s.rng=rng.str();
  s.config=config_;s.quick=quick_;s.layer2=layer2_;s.layer3=layer3_;s.active=active_;s.cave=cave_;s.freeRefresh=freeRefresh_;
  s.restId=restId_;s.token=token_;s.stones=stones_;s.pendingXp=pendingXp_;s.paidTreasure=paidTreasure_;
  s.shop=shop_;s.hidden=hidden_;s.reforges=reforges_;s.reforge=reforge_;s.treasure=treasure_;s.freeTreasures=freeTreasures_;s.undo=undo_;
  return s;
}
bool Economy::RestoreState(const EconomyState& s,const Progression& p) {
  if(s.version!=1||s.stones<0||s.pendingXp<0||s.paidTreasure<0||s.paidTreasure>5||
     (s.layer3&&!s.layer2)||(s.active&&!p.IsResting())||(s.quick&&s.cave)||
     (s.freeRefresh&&!s.cave)||(!s.layer2&&s.stones!=0)||
     (!s.active&&(!s.shop.empty()||!s.undo.empty()||s.reforge||s.treasure||s.pendingXp))||
     (s.active&&s.restId==0)||s.rng.size()>20000)return false;
  auto positive=[](int x){return x>0&&x<=1000000;};
  auto nonnegative=[](int x){return x>=0&&x<=1000000;};
  const auto& c=s.config;
  if(!std::all_of(c.killStones.begin(),c.killStones.end(),nonnegative)||
     !std::all_of(c.refineStones.begin(),c.refineStones.end(),nonnegative)||
     !std::all_of(c.shopPrices.begin(),c.shopPrices.end(),positive)||
     !positive(c.refreshPrice)||!positive(c.reforgeBase)||!positive(c.reforgeStep)||
     !positive(c.treasureBase)||!positive(c.treasureStep)||!positive(c.pityWaves)||
     c.traditionalGoods<3||c.traditionalGoods>5||c.paidTreasureCap<0||c.paidTreasureCap>5||
     s.paidTreasure>c.paidTreasureCap)return false;
  if(!std::isfinite(c.rarityWeight)||c.rarityWeight<1||c.rarityWeight>10)return false;
  for(auto w:c.treasureWeights)if(!std::isfinite(w)||w<=0||w>1000000)return false;
  auto affixValid=[](const Affix& a){return int(a.kind)>=0&&int(a.kind)<int(AffixKind::Count)&&std::isfinite(a.value);};
  auto relicValid=[&](const Relic& r){return r.id>0&&r.id<p.nextRelic_&&r.id<std::numeric_limits<EntityId>::max()&&int(r.rarity)>=0&&int(r.rarity)<4&&
    r.catalogId>=0&&r.catalogId<int(RelicCatalog().size())&&r.affixes.size()<=64&&std::all_of(r.affixes.begin(),r.affixes.end(),affixValid);};
  if(s.shop.size()>5||s.undo.size()>100000||s.hidden.size()>1000000)return false;
  std::set<std::uint64_t> tokens;std::set<EntityId> stock;
  for(const auto& g:s.shop)if(!relicValid(g.relic)||g.relic.rarity==Rarity::Dao||g.price<=0||g.token==0||g.token>s.token||
    !tokens.insert(g.token).second||!stock.insert(g.relic.id).second)return false;
  std::set<EntityId> owned;
  for(auto* v:{&p.GetState().equipped,&p.GetState().bag})for(const auto& r:*v)owned.insert(r.id);
  for(const auto& g:s.shop)if(!g.sold&&owned.count(g.relic.id))return false;
  if(s.treasure&&(owned.count(s.treasure->id)||stock.count(s.treasure->id)))return false;
  std::int64_t xp=0;
  for(const auto& u:s.undo) {
    if(u.amount<0||(u.reward!=RefineReward::Experience&&u.reward!=RefineReward::Stones)||u.relics.empty()||
       (u.reward==RefineReward::Stones&&!s.layer2))return false;
    std::int64_t expected=0;
    for(const auto& r:u.relics) {
      if(!relicValid(r)||r.locked||!owned.insert(r.id).second||
         (s.treasure&&r.id==s.treasure->id))return false;
      expected+=u.reward==RefineReward::Stones?c.refineStones[int(r.rarity)]:RefineValue(r.rarity);
    }
    if(expected!=u.amount)return false;
    if(u.reward==RefineReward::Experience)xp+=u.amount;
  }
  if(xp!=s.pendingXp)return false;
  for(const auto& h:s.hidden)if(!owned.count(h.first)||h.second.empty()||h.second.size()>64||!std::all_of(h.second.begin(),h.second.end(),affixValid))return false;
  for(const auto& n:s.reforges)if(n.first==0||n.second<0)return false;
  if(s.treasure&&!relicValid(*s.treasure))return false;
  if(s.reforge) {
    auto equal=[](const Affix& a,const Affix& b){return a.kind==b.kind&&a.code==b.code&&a.value==b.value;};
    const auto& r=*s.reforge;const Relic* found=nullptr;
    for(auto* v:{&p.GetState().equipped,&p.GetState().bag})for(const auto& item:*v)if(item.id==r.relic)found=&item;
    if(!found||r.index>=found->affixes.size()||!affixValid(r.oldAffix)||!affixValid(r.newAffix)||
       !Eligible(r.oldAffix)||!Eligible(r.newAffix)||r.paid<=0||!s.layer3||!s.cave||
       !equal(found->affixes[r.index],r.oldAffix)||!ValidReplacement(*found,r.index,r.newAffix))return false;
  }
  for(auto id:s.freeTreasures)if(id==0||id>s.restId)return false;
  std::istringstream rng(s.rng);rng.imbue(std::locale::classic());std::mt19937 parsed;
  if(!(rng>>parsed))return false;
  rng>>std::ws;if(!rng.eof())return false;
  Economy next;next.config_=s.config;next.rng_=parsed;next.quick_=s.quick;next.layer2_=s.layer2;next.layer3_=s.layer3;
  next.active_=s.active;next.cave_=s.cave;next.freeRefresh_=s.freeRefresh;next.restId_=s.restId;next.token_=s.token;
  next.stones_=s.stones;next.pendingXp_=s.pendingXp;next.paidTreasure_=s.paidTreasure;next.shop_=s.shop;next.hidden_=s.hidden;
  next.reforges_=s.reforges;next.reforge_=s.reforge;next.treasure_=s.treasure;next.freeTreasures_=s.freeTreasures;next.undo_=s.undo;
  *this=std::move(next);return true;
}

}
