#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MVP/Simulation.h"
#include "MVP/DywxMVPProfile.h"
#include "DywxMVPWorld.generated.h"

class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class USoundBase;
class USoundConcurrency;

// A single ticking owner. The simulation is authoritative; all component collision is disabled.
UCLASS()
class DYWXUI_API ADywxMVPWorld : public AActor
{
    GENERATED_BODY()
public:
    ADywxMVPWorld();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void StartRun(dywx::mvp::HeroId Hero, uint64 Seed);
    void StopRun();
    bool IsRunActive() const { return bActive; }
    bool IsWindowForeground() const { return bWindowForeground; }
    bool ConsumeFocusPauseRequest() { const bool Requested=bFocusPauseRequested; bFocusPauseRequested=false; return Requested; }
    void SetInput(const dywx::mvp::InputState& Value) { Input = Value; }
    dywx::mvp::Simulation& GetSimulation() { return Simulation; }
    const dywx::mvp::Snapshot& GetSnapshot() const { return Simulation.GetSnapshot(); }
private:
    struct FBatch { UInstancedStaticMeshComponent* Component = nullptr; TArray<FTransform> Pending; int32 Allocated = 0; bool Dynamic = true; };
    struct FTransient { FVector Position; float Age=0; bool Pickup=false; };
    dywx::mvp::Simulation Simulation;
    FDywxMVPProfile Profile;
    bool bProfileLoaded=false;
    float ProfileElapsed=0;
    int32 ProfileWave=-1;
    dywx::mvp::RunPhase ProfilePhase=dywx::mvp::RunPhase::Title;
    void SaveProfile();
    dywx::mvp::InputState Input;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> OwnedMeshes;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> OwnedMaterials;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> OwnedBatches;
    UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TObjectPtr<USoundConcurrency> SoundConcurrency;
    TMap<FName,UStaticMesh*> Meshes;
    TMap<FName,FBatch> Batches;
    TMap<uint64,FVector> PreviousEnemies, PreviousPickups;
    TMap<uint64,float> SwarmWalkPhases;
    float HeroWalkPhase=0;
    FVector PreviousPlayer=FVector::ZeroVector;
    bool bHavePreviousPlayer=false;
    TArray<FTransient> Transients;
    TWeakObjectPtr<AActor> PreviousView;
    bool bActive=false, bSceneBuilt=false;
    bool bWindowForeground=true, bFocusPauseRequested=false;
    void RefreshRunBudget();
    float SavedFPS=0, AppliedFPS=-1, VisualTime=0;
    uint64 LastGeneration=0, LastShots=0, LastEffect=0;
    float LastHP=0, LastDash=0, SoundCooldown=0;
    int32 LastLevel=1;
    dywx::mvp::RunPhase LastPhase=dywx::mvp::RunPhase::Title;
    void BuildScene();
    void LoadGeometry();
    void SetupVFX();
    void RenderVFX(const dywx::mvp::Snapshot& Snapshot,bool& NewPalm,bool& NewThunder,uint64& MaxEffect);
    void Render(float DeltaSeconds);
    void Add(FName Batch, const FVector& Position, const FVector& Scale=FVector::OneVector, float Yaw=0);
    void Flush();
    void MakeBatch(FName Name, FName MeshName, FLinearColor Tint, bool bDynamic=true, bool bEmissive=false);
    UStaticMesh* BuildMesh(FName Name, const TArray<FVector3f>& Vertices, const TArray<int32>& Indices, const TArray<FVector4f>& Colors, const TArray<FVector3f>& VertexNormals = {});
};
