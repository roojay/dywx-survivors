#include "MVP/Economy.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace dywx::mvp;
static void RoundTrip(const Economy& source,const Progression& p) {
  Economy copy; assert(copy.RestoreState(source.ExportState(),p));
  const auto a=source.ExportState(),b=copy.ExportState();
  assert(a.rng==b.rng&&a.stones==b.stones&&a.pendingXp==b.pendingXp&&a.active==b.active);
  assert(a.shop.size()==b.shop.size()&&a.undo.size()==b.undo.size()&&a.hidden.size()==b.hidden.size());
}
int main() {
  // The final available token range is usable; only actual exhaustion rejects.
  for(bool quick:{false,true})for(int goods=3;goods<=5;++goods) {
    Progression p; p.Reset(11,HeroId::Body); p.BeginRest();
    Economy e; EconomyConfig config; config.traditionalGoods=goods;
    e.Reset(12,quick,true,true,config);
    auto last=e.ExportState(); auto count=unsigned(quick?3:goods);
    last.token=std::numeric_limits<std::uint64_t>::max()-count;
    assert(e.RestoreState(last,p)); assert(e.BeginRest(p,1,true));
    assert(e.Shop().size()==count&&e.Shop().back().token==std::numeric_limits<std::uint64_t>::max());
    RoundTrip(e,p);
    e.EndRest(p); assert(!e.BeginRest(p,2,true));
    assert(!e.ExportState().active&&e.Shop().empty());
  }
  for(int layer=1;layer<=3;++layer)for(bool quick:{false,true}) {
    Progression p; p.Reset(61,HeroId::Body); p.ConfigureFullGame(layer);
    Economy e; e.Reset(62,quick,layer>=2,layer>=3); RoundTrip(e,p);
    for(int i=0;i<10;++i)e.ReceiveRelic(p,p.GenerateRelic(Rarity::Spirit),true);
    e.ReceiveRelic(p,p.GenerateRelic(Rarity::Immortal)); RoundTrip(e,p);
    for(std::uint64_t rest=1;rest<=3;++rest) {
      p.BeginRest(); assert(e.BeginRest(p,rest,false)); RoundTrip(e,p);
      if(!quick){assert(e.EnterCave(p,true)); RoundTrip(e,p);}
      if(layer>=2) {
        assert(e.CreditStones(1000));
        assert(e.Buy(p,e.Shop().front().token)); RoundTrip(e,p);
        auto id=p.GetState().bag.front().id;
        assert(e.Refine(p,{id},RefineReward::Stones)); RoundTrip(e,p);
        assert(e.UndoRefine(p)); RoundTrip(e,p);
      }
      auto id=p.GetState().bag.front().id;
      assert(e.Refine(p,{id},RefineReward::Experience)); RoundTrip(e,p);
      assert(e.UndoRefine(p)); RoundTrip(e,p);
      assert(e.Refine(p,{id},RefineReward::Experience)); RoundTrip(e,p);
      if(layer==3) {
        assert(e.OpenTreasure(p,false,true)); RoundTrip(e,p);
        assert(e.RevealTreasure(p)); RoundTrip(e,p);
        if(!quick) {
          auto equipped=p.GetState().equipped.front().id;
          assert(e.BeginReforge(p,equipped,0)); RoundTrip(e,p);
          assert(e.ResolveReforge(p,false)); RoundTrip(e,p);
        }
      }
      e.EndRest(p); RoundTrip(e,p); p.EndRest(); RoundTrip(e,p);
    }
  }
  Progression p; p.Reset(41,HeroId::Body);
  Economy e; e.Reset(42,false,true,true);
  for(int i=0;i<10;++i)e.ReceiveRelic(p,p.GenerateRelic(Rarity::Spirit),true);
  p.BeginRest(); assert(e.BeginRest(p,1,true)); assert(e.CreditStones(1000));
  auto id=p.GetState().bag.front().id;
  assert(e.Refine(p,{id},RefineReward::Stones));
  auto valid=e.ExportState(); Economy restored; assert(restored.RestoreState(valid,p));
  auto bad=valid; bad.undo.back().amount=0;
  if(restored.RestoreState(bad,p)) {
    int before=restored.Stones(); assert(restored.UndoRefine(p));
    std::cout << "BUG: restored zero-cost undo returns refined item while retaining " << before << " stones\n";
    return 1;
  }
  bad=valid; bad.token=std::numeric_limits<std::uint64_t>::max();
  Economy exhausted; assert(exhausted.RestoreState(bad,p));
  assert(!exhausted.Refresh(p)); assert(exhausted.Stones()==e.Stones());
  assert(exhausted.Shop().front().token==e.Shop().front().token);
  bad=valid; bad.shop.front().relic.id=1000000000;
  assert(!restored.RestoreState(bad,p));
  bad=valid; bad.undo.back().relics.front().locked=true;
  assert(!restored.RestoreState(bad,p));
  bad=valid; bad.active=false;
  assert(!restored.RestoreState(bad,p));
  assert(restored.Stones()==e.Stones());
  auto twin=p;
  assert(restored.UndoRefine(p)); assert(e.UndoRefine(twin));
  assert(restored.Refresh(p)); assert(e.Refresh(twin));
  assert(restored.ExportState().rng==e.ExportState().rng);
  for(std::size_t i=0;i<e.Shop().size();++i) {
    const auto& a=restored.Shop()[i]; const auto& b=e.Shop()[i];
    assert(a.token==b.token&&a.relic.id==b.relic.id&&a.relic.catalogId==b.relic.catalogId);
    assert(a.relic.affixes.size()==b.relic.affixes.size());
    for(std::size_t j=0;j<a.relic.affixes.size();++j)
      assert(a.relic.affixes[j].kind==b.relic.affixes[j].kind&&a.relic.affixes[j].value==b.relic.affixes[j].value);
  }
  std::cout << "Economy boundary tests passed\n";
}
