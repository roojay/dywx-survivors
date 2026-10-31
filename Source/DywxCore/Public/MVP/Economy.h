#pragma once
#include "MVP/Export.h"
#include "MVP/Progression.h"
#include <map>
#include <set>
namespace dywx::mvp {
enum class RefineReward { Experience, Stones };
struct DYWXCORE_API EconomyConfig {
  // Fixed v0.1 rates. Other prices/probabilities are provisional tuning values.
  std::array<int,4> killStones{1,4,20,100};
  std::array<int,4> refineStones{5,15,50,120};
  std::array<int,3> shopPrices{10,30,110};
  int refreshPrice=15, reforgeBase=20, reforgeStep=15;
  int treasureBase=30, treasureStep=20, paidTreasureCap=5;
  int traditionalGoods=4, pityWaves=20;
  float rarityWeight=1; // campaign risk, finite [1,10], applied to non-common chest weights
  std::array<double,4> treasureWeights{70,29.05,.6,.35};
};
struct DYWXCORE_API ShopGood { std::uint64_t token=0; Relic relic; int price=0; bool sold=false; };
struct DYWXCORE_API EconomyRefinePreview { std::vector<EntityId> ids; int amount=0; RefineReward reward=RefineReward::Experience; };
struct DYWXCORE_API ReforgeOffer { EntityId relic=0; std::size_t index=0; Affix oldAffix, newAffix; int paid=0; };
struct DYWXCORE_API EconomyUndo { std::vector<Relic> relics; int amount=0; RefineReward reward=RefineReward::Experience; };
struct DYWXCORE_API EconomyState {
  int version=1;
  EconomyConfig config;
  std::string rng;
  bool quick=true, layer2=false, layer3=false, active=false, cave=false, freeRefresh=false;
  std::uint64_t restId=0, token=0;
  int stones=0, pendingXp=0, paidTreasure=0;
  std::vector<ShopGood> shop;
  std::map<EntityId,std::vector<Affix>> hidden;
  std::map<EntityId,int> reforges;
  std::optional<ReforgeOffer> reforge;
  std::optional<Relic> treasure;
  std::set<std::uint64_t> freeTreasures;
  std::vector<EconomyUndo> undo;
};
class DYWXCORE_API Economy {
public:
  void Reset(std::uint32_t seed, bool quick, bool layer2, bool layer3, EconomyConfig config={});
  // restId must be monotonically increasing per run; repeats cannot regenerate stock.
  bool BeginRest(Progression&, std::uint64_t restId, bool cave, bool freeRefresh=false);
  void EndRest(Progression&);
  bool CommitRestExperience(Progression&); // suspend preparation; XP undo becomes final
  bool EnterCave(Progression&, bool freeRefresh=false);
  void EndRun(); // Currency reset, identification metadata retained for result display.
  bool ChooseBossReward(Progression&, std::size_t index);
  EconomyState ExportState() const;
  bool RestoreState(const EconomyState&, const Progression&); // Validate then atomic replace.
  int Stones() const { return stones_; }
  int PendingXp() const { return pendingXp_; }
  int PaidTreasureCount() const { return paidTreasure_; }
  void CreditKill(EnemyTier);
  bool CreditStones(int amount); // Already-authorized event rewards only; layer gated.
  const std::vector<ShopGood>& Shop() const { return shop_; }
  bool DiscoverCurrentShop(Progression&); // Explicit presentation command, never a read-side effect.
  bool Buy(Progression&, std::uint64_t token);
  bool Refresh(Progression&);
  EconomyRefinePreview PreviewRefine(const Progression&, const std::vector<EntityId>&, RefineReward) const;
  bool Refine(Progression&, const std::vector<EntityId>&, RefineReward);
  bool UndoRefine(Progression&);
  int ReforgePrice(EntityId) const;
  bool BeginReforge(Progression&, EntityId, std::size_t affix);
  bool ResolveReforge(Progression&, bool keepNew);
  const std::optional<ReforgeOffer>& PendingReforge() const { return reforge_; }
  std::array<double,4> TreasureProbabilities(const Progression&) const;
  int TreasurePrice() const;
  // A free opening requires the caller to have selected this rest's treasure encounter.
  bool OpenTreasure(Progression&, bool paid, bool treasureEncounter=false);
  bool HasTreasureToReveal() const { return treasure_.has_value(); }
  std::optional<Relic> RevealTreasure(Progression&);
  void ReceiveRelic(Progression&, Relic, bool identified=false);
  bool IsUnidentified(EntityId id) const { return hidden_.count(id)!=0; }
  bool Identify(Progression&, EntityId);
private:
  EconomyConfig config_;
  std::mt19937 rng_{1};
  bool quick_=true, layer2_=false, layer3_=false, active_=false, cave_=false, freeRefresh_=false;
  std::uint64_t restId_=0, token_=0;
  int stones_=0, pendingXp_=0, paidTreasure_=0;
  std::vector<ShopGood> shop_;
  std::map<EntityId,std::vector<Affix>> hidden_;
  std::map<EntityId,int> reforges_;
  std::optional<ReforgeOffer> reforge_;
  std::optional<Relic> treasure_;
  std::set<std::uint64_t> freeTreasures_;
  std::vector<EconomyUndo> undo_;
  bool Rest(const Progression&) const;
  void GenerateShop(Progression&);
  Relic* Find(Progression&,EntityId);
};
}
