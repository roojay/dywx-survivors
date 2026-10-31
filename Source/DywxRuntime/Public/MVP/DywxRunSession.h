#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MVP/Simulation.h"
#include "MVP/DywxMVPProfile.h"
#include "DywxRunSession.generated.h"

// World-lifetime standalone authority. Presentation never owns or mutates Simulation.
// Disk profile is an adapter owned here for now, not a replicated/player identity service.
// Network play deliberately fails closed until server/profile ownership is implemented.
UCLASS()
class DYWXRUNTIME_API UDywxRunSession : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;
    bool IsStandaloneSession() const;
    void InitializeProfile();
    void StartRun(dywx::mvp::HeroId Hero, uint64 Seed);
    bool StartFullRun(int32 Hero, int32 Map, int32 Mode, uint32 RiskMask = 0);
    void StopRun();
    bool IsRunActive() const { return bActive && IsStandaloneSession(); }
    // Local read facade only, NOT a network-safe DTO: future replication must filter
    // hidden economy/affix/RNG state instead of serializing this object.
    const dywx::mvp::Simulation* GetSimulation() const { return IsStandaloneSession() ? &Simulation : nullptr; }
    const dywx::mvp::Snapshot& GetSnapshot() const { return Simulation.GetSnapshot(); }
    const dywx::mvp::CampaignProfile& GetCampaignProfile() const { return Profile.GetCampaign(); }
    bool PurchaseLegacy(dywx::mvp::LegacyUnlock Unlock);
    bool PurchasePermanent(dywx::mvp::PermanentStat Stat);
    bool SaveRun();
    bool ResumeRun(int32 Hero);
    bool HasSavedRun(int32 Hero) const;
    // Weak UObject-bound callback lets the local controller pause/clear input before
    // integration without pulling Slate or controller types into this runtime module.
    void SetBeforeSimulationTick(FSimpleDelegate Callback) { BeforeSimulationTick = MoveTemp(Callback); }
    void ClearBeforeSimulationTick() { BeforeSimulationTick.Unbind(); }
    void SetInput(const dywx::mvp::InputState& Value) { Input = IsRunActive() ? Value : dywx::mvp::InputState{}; }
    void Pause(bool Paused) { if (IsRunActive()) { Input = {}; Simulation.Pause(Paused); } }
    bool ChooseOpportunity(unsigned Index) { return IsRunActive() && Simulation.ChooseOpportunity(Index); }
    bool ChooseBossReward(std::size_t Index) { return IsRunActive() && Simulation.ChooseBossReward(Index); }
    bool FinalizeRestExperience() { return IsRunActive() && Simulation.FinalizeRestExperience(); }
    bool BuyGood(std::uint64_t Token) { return IsRunActive() && Simulation.BuyGood(Token); }
    bool RefreshShop() { return IsRunActive() && Simulation.RefreshShop(); }
    bool BeginReforge(dywx::mvp::EntityId Id, std::size_t Affix) { return IsRunActive() && Simulation.BeginReforge(Id, Affix); }
    bool ResolveReforge(bool KeepNew) { return IsRunActive() && Simulation.ResolveReforge(KeepNew); }
    bool OpenTreasure(bool Paid = true) { return IsRunActive() && Simulation.OpenTreasure(Paid); }
    bool RevealTreasure() { return IsRunActive() && Simulation.RevealTreasure(); }
    bool IdentifyRelic(dywx::mvp::EntityId Id) { return IsRunActive() && Simulation.IdentifyRelic(Id); }
    bool Refine(const std::vector<dywx::mvp::EntityId>& Ids, dywx::mvp::RefineReward Reward) { return IsRunActive() && Simulation.Refine(Ids, Reward); }
    bool UndoRefine() { return IsRunActive() && Simulation.UndoRefine(); }
    bool SealUpgrade(std::size_t Index) { return IsRunActive() && Simulation.SealUpgrade(Index); }
    bool SkipUpgrade() { return IsRunActive() && Simulation.SkipUpgrade(); }
    bool ChooseUpgrade(std::size_t Index) { return IsRunActive() && Simulation.ChooseUpgrade(Index); }
    bool Reroll() { return IsRunActive() && Simulation.Reroll(); }
    bool BeginNextWave() { return IsRunActive() && Simulation.BeginNextWave(); }
    bool Equip(dywx::mvp::EntityId Id, std::size_t Slot) { return IsRunActive() && Simulation.Equip(Id, Slot); }
    bool Unequip(std::size_t Slot) { return IsRunActive() && Simulation.Unequip(Slot); }
    bool SetLocked(dywx::mvp::EntityId Id, bool Locked) { return IsRunActive() && Simulation.SetLocked(Id, Locked); }
    bool SortBag() { return IsRunActive() && Simulation.SortBag(); }
    bool Fuse(dywx::mvp::SpellId Id) { return IsRunActive() && Simulation.Fuse(Id); }
private:
    void ActivateRun();
    bool SaveProfile();
    void ObserveRunInto(FDywxMVPProfile& CandidateProfile) const;
    FSimpleDelegate BeforeSimulationTick;
    dywx::mvp::Simulation Simulation;
    dywx::mvp::InputState Input;
    FDywxMVPProfile Profile;
    bool bActive = false, bProfileLoaded = false;
    float ProfileElapsed = 0;
    int32 ProfileWave = -1;
    dywx::mvp::RunPhase ProfilePhase = dywx::mvp::RunPhase::Title;
};
