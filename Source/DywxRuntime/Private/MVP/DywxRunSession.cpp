#include "MVP/DywxRunSession.h"
#include "MVP/SaveCodec.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
using namespace dywx::mvp;
DEFINE_LOG_CATEGORY_STATIC(LogDywxRunSession, Log, All);

bool UDywxRunSession::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE;
}
bool UDywxRunSession::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return Super::ShouldCreateSubsystem(Outer) && World && World->GetNetMode() == NM_Standalone;
}
bool UDywxRunSession::IsStandaloneSession() const
{
    const UWorld* World = GetWorld();
    return World && DoesSupportWorldType(World->WorldType) && World->GetNetMode() == NM_Standalone;
}
bool UDywxRunSession::IsTickable() const
{
    return Super::IsTickable() && IsRunActive();
}
TStatId UDywxRunSession::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UDywxRunSession, STATGROUP_Tickables);
}
void UDywxRunSession::Deinitialize()
{
    if (bActive) StopRun();
    else if (bProfileLoaded && IsStandaloneSession()) SaveProfile();
    BeforeSimulationTick.Unbind();
    Super::Deinitialize();
}
void UDywxRunSession::InitializeProfile()
{
    if (!IsStandaloneSession()) return;
    if(bProfileLoaded) return;
    const bool Loaded=Profile.Load(); bProfileLoaded=true;
    if(!Loaded && !Profile.IsWritable())
        UE_LOG(LogDywxRunSession,Error,TEXT("Local profile is unreadable. Keeping the existing file intact; account mutations and campaign start are blocked."));
}
void UDywxRunSession::StartRun(HeroId Hero,uint64 Seed)
{
    if (!IsStandaloneSession()) return;
    if(bProfileLoaded && !SaveProfile()) return;
    InitializeProfile();
    Simulation.Start(Seed,Hero);
    Profile.Apply(Simulation.GetProgression()); Simulation.RefreshProgression();
    ActivateRun();
}
bool UDywxRunSession::StartFullRun(int32 Hero,int32 Map,int32 Mode,uint32 RiskMask)
{
    if (!IsStandaloneSession()) return false;
    InitializeProfile();
    if(!Profile.IsWritable() || Hero<0 || Hero>=3 || Map<0 || Map>=3 || Mode<0 || Mode>=3) return false;
    // One in-progress run per hero. A new start must not silently replace it.
    if(Profile.HasCheckpoint(HeroId(Hero))) return false;
    if(!SaveProfile()) return false;
    // Validate against the account before reserving a durable id. No live run is
    // replaced and no mesh work happens until persistence has succeeded.
    dywx::mvp::Simulation Candidate;
    const uint64 Proposed=Profile.GetCampaign().nextRunId;
    const uint64 Seed=Proposed*0x9e3779b97f4a7c15ULL+uint64(Map+1);
    if(!Candidate.StartCampaign(Profile.GetCampaign(),GameMode(Mode),HeroId(Hero),Seed,Proposed,CaveId(Map),RiskMask)) return false;
    const uint64 RunId=Profile.AllocateRunId();
    if(!RunId) return false;
    if(!Candidate.StartCampaign(Profile.GetCampaign(),GameMode(Mode),HeroId(Hero),Seed,RunId,CaveId(Map),RiskMask)) return false;
    Profile.Apply(Candidate.GetProgression()); Candidate.RefreshProgression();
    Simulation=MoveTemp(Candidate); ActivateRun(); return true;
}
bool UDywxRunSession::PurchaseLegacy(LegacyUnlock Unlock)
{
    if (!IsStandaloneSession()) return false;
    InitializeProfile(); if(bActive || !Profile.IsWritable()) return false;
    auto Candidate=Profile;
    if(!PurchaseUnlock(Candidate.MutableCampaign(),Unlock) || !Candidate.Save()) return false;
    Profile=MoveTemp(Candidate); return true;
}
bool UDywxRunSession::PurchasePermanent(PermanentStat Stat)
{
    if (!IsStandaloneSession()) return false;
    InitializeProfile(); if(bActive || !Profile.IsWritable()) return false;
    auto Candidate=Profile;
    if(!PurchaseBonus(Candidate.MutableCampaign(),Stat) || !Candidate.Save()) return false;
    Profile=MoveTemp(Candidate); return true;
}
bool UDywxRunSession::HasSavedRun(int32 Hero) const
{
    return bProfileLoaded && Hero>=0 && Hero<3 && Profile.HasCheckpoint(HeroId(Hero));
}
bool UDywxRunSession::SaveRun()
{
    if (!IsStandaloneSession()) return false;
    if(!bActive || !Profile.IsWritable()) return false;
    // Prepare a projected boundary on copies; a failed write must not commit XP
    // or consume pending choices in the live, possibly paused intermission.
    RunCheckpoint Checkpoint; std::string Bytes;
    if(!Simulation.PrepareCheckpoint(Checkpoint) || !SaveCodec::EncodeCheckpoint(Checkpoint,Bytes)) return false;
    return Profile.SaveCheckpoint(HeroId(Checkpoint.campaign.hero),Bytes);
}
bool UDywxRunSession::ResumeRun(int32 Hero)
{
    if (!IsStandaloneSession()) return false;
    InitializeProfile(); if(Hero<0 || Hero>=3 || !Profile.IsWritable()) return false;
    if(!SaveProfile()) return false;
    std::string Bytes; RunCheckpoint Checkpoint;
    if(!Profile.GetCheckpoint(HeroId(Hero),Bytes) || !SaveCodec::DecodeCheckpoint(Bytes,Checkpoint)) return false;
    dywx::mvp::Simulation Candidate;
    if(!Candidate.RestoreCheckpoint(Checkpoint,Profile.GetCampaign())) return false;
    // Consumption is durable before activation: one checkpoint cannot duplicate rewards.
    std::string Consumed;
    if(!Profile.ConsumeCheckpoint(HeroId(Hero),Consumed)) return false;
    Simulation=MoveTemp(Candidate); ActivateRun(); return true;
}
bool UDywxRunSession::SaveProfile()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(DywxSession_SaveProfile);
    if (!IsStandaloneSession()) return false;
    if(!bProfileLoaded || !Profile.IsWritable()) return false;
    if(GetSnapshot().phase==RunPhase::Title) return true; // No run discoveries/pity to observe yet.
    auto CandidateProfile=Profile;
    CandidateProfile.Observe(GetSnapshot().progression);
    if(Simulation.IsCampaign()) {
        const auto& Seen=Simulation.GetAnomalies().State().encountered;
        for(std::size_t I=0;I<Seen.size();I++) if(Seen[I]) CandidateProfile.MutableCampaign().seenAnomalies[I]=true;
        bool BossEncountered=GetSnapshot().phase==RunPhase::Boss || GetSnapshot().phase==RunPhase::Won || Simulation.GetCampaign().GetState().bosses>0;
        for(const auto& Enemy:GetSnapshot().enemies) BossEncountered|=Enemy.role==EnemyRole::Boss;
        const int32 Cave=static_cast<int32>(Simulation.GetArena().cave);
        if(BossEncountered && Cave>=0 && Cave<3) CandidateProfile.MutableCampaign().seenBosses[static_cast<std::size_t>(Cave)]=true;
    }
    auto CandidateSimulation=Simulation;
    const bool Settled=CandidateSimulation.IsCampaign() && CandidateSimulation.SettleCampaign(CandidateProfile.MutableCampaign());
    if(!CandidateProfile.Save()) {
        UE_LOG(LogDywxRunSession,Warning,TEXT("Account save failed; settlement remains pending and will retry at the next boundary."));
        // Do not hammer a failed disk every frame; explicit user commands still retry immediately.
        ProfileElapsed=0; ProfileWave=GetSnapshot().wave; ProfilePhase=GetSnapshot().phase;
        return false;
    }
    Profile=MoveTemp(CandidateProfile);
    if(Settled) Simulation=MoveTemp(CandidateSimulation);
    ProfileElapsed=0; ProfileWave=GetSnapshot().wave; ProfilePhase=GetSnapshot().phase;
    return true;
}

void UDywxRunSession::ActivateRun()
{
    ProfileElapsed=0; ProfileWave=-1; ProfilePhase=RunPhase::Title;
    Input={}; bActive=true;
}
void UDywxRunSession::StopRun()
{
    if (!bActive) return;
    SaveProfile(); bActive=false; Input={};
}
void UDywxRunSession::Tick(float Dt)
{
    if (!IsRunActive()) return;
    BeforeSimulationTick.ExecuteIfBound();
    if (!IsRunActive()) return;
    // One fixed-step owner; no Actor/Controller/UI is permitted to integrate combat.
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(DywxSession_SimulationStep);
        Simulation.Step(FMath::Clamp(Dt,0.f,.1f),Input);
    }
    const auto& S=GetSnapshot();
    ProfileElapsed+=FMath::Clamp(Dt,0.f,.1f);
    if(ProfileElapsed>=10 || ProfileWave!=S.wave || ProfilePhase!=S.phase) SaveProfile();
}
