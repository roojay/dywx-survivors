#pragma once
#include "CoreMinimal.h"
#include "MVP/SaveCodec.h"
class DYWXRUNTIME_API FDywxMVPProfile {
public:
  FDywxMVPProfile();
  bool Load();
  bool IsWritable() const { return bWritable; }
  void Apply(dywx::mvp::Progression&) const;
  void Observe(const dywx::mvp::ProgressionState&);
  bool Save();
  const dywx::mvp::ProgressionState& GetKnown() const { return Store.Data().known; }
  const dywx::mvp::CampaignProfile& GetCampaign() const { return Store.Data().campaign; }
  dywx::mvp::CampaignProfile& MutableCampaign() { return Store.MutableCampaign(); }
  std::uint64_t AllocateRunId();
  bool SaveCheckpoint(dywx::mvp::HeroId,const std::string&);
  bool HasCheckpoint(dywx::mvp::HeroId) const;
  bool GetCheckpoint(dywx::mvp::HeroId,std::string&) const;
  bool ConsumeCheckpoint(dywx::mvp::HeroId,std::string&);
private:
  dywx::mvp::ProfileStore Store;
  bool bWritable=false;
};
