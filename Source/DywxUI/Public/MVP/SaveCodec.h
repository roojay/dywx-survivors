#pragma once
#include "MVP/Campaign.h"
#include "MVP/Economy.h"
#include "MVP/Arena.h"
#include <functional>
namespace dywx::mvp {
// A completed intermission projected to the next wave/boss entry; no combat entities.
struct CombatBoundaryState {
  std::array<float,7> procCooldowns{};
  float speedBuff=0, powerBuff=0, shieldTime=0, bodyProcCooldown=0, critCooldown=0, resetTimer=0, terrainTimer=0;
  bool lowHpUsed=false, lowHpActive=false;
};
struct RunCheckpoint {
  Progression progression;
  Economy economy;
  CampaignRunState campaign;
  CampaignConfig campaignConfig;
  RunConfig runConfig;
  std::uint64_t seed=1, simulationRng=1, nextEntityId=1, restSerial=0;
  int realm=0;
  PlayerState player;
  RunStatistics statistics;
  ArenaLayout arena;
  AnomalySystem anomalies;
  CombatBoundaryState combat;
  std::array<float,kSpellCount> spellCooldowns{};
  std::array<int,kSpellCount> casts{};
};
struct ProfileData {
  ProgressionState known;
  CampaignProfile campaign;
  std::array<std::string,3> checkpoints;
};
class SaveArchive;
class SaveCodec {
public:
  static constexpr std::size_t MaxBytes=32*1024*1024;
  static bool EncodeCheckpoint(const RunCheckpoint&, std::string&);
  static bool DecodeCheckpoint(const std::string&, RunCheckpoint&);
  static bool ValidateCheckpoint(const RunCheckpoint&);
  static bool EncodeProfile(const ProfileData&, std::string&);
  static bool DecodeProfile(const std::string&, ProfileData&);
private:
  static void ProgressionFields(SaveArchive&, Progression&);
  static bool ValidateProgression(const Progression&);
  static void AnomalyFields(SaveArchive&, AnomalySystem&);
};
// Persistence boundary shared by UE and standalone failure-injection tests.
// Writer must atomically replace durable bytes; false leaves the old file intact.
class ProfileStore {
public:
  using Writer=std::function<bool(const std::string&)>;
  explicit ProfileStore(Writer writer={}) : writer_(std::move(writer)) {}
  void SetWriter(Writer writer) { writer_=std::move(writer); }
  bool Load(const std::string&);
  bool ImportLegacy(const ProgressionState&);
  const ProfileData& Data() const { return data_; }
  CampaignProfile& MutableCampaign() { dirty_=true;return data_.campaign; }
  void Observe(const ProgressionState&);
  bool Save();
  std::uint64_t AllocateRunId();
  bool SaveCheckpoint(HeroId,const std::string&);
  bool HasCheckpoint(HeroId) const;
  bool GetCheckpoint(HeroId,std::string&) const;
  bool ConsumeCheckpoint(HeroId,std::string&);
private:
  ProfileData data_;
  Writer writer_;
  std::string lastCommitted_;
  bool dirty_=false;
  bool Commit(ProfileData);
};
}
