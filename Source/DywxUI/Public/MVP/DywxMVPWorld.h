#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MVP/Simulation.h"
#include "MVP/DywxRunSession.h"
#include "DywxMVPWorld.generated.h"

class APlayerController;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class USoundBase;
class USoundConcurrency;

// Local camera/mesh/audio presentation only. All component collision is disabled.
UCLASS()
class DYWXUI_API ADywxMVPWorld : public AActor
{
    GENERATED_BODY()
public:
    ADywxMVPWorld();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void InitializePresentation(UDywxRunSession* InSession, APlayerController* InController);
    void ActivateRun();
    void StopRun(); // Presentation only; session lifetime belongs to the world.
    const dywx::mvp::Snapshot& GetSnapshot() const { return Session->GetSnapshot(); }
private:
    struct FBatch { UInstancedStaticMeshComponent* Component = nullptr; TArray<FTransform> Pending; int32 Allocated = 0; bool Dynamic = true; };
    struct FTransient { FVector Position; float Age=0; bool Pickup=false; };
    UPROPERTY(Transient) TObjectPtr<UDywxRunSession> Session;
    TWeakObjectPtr<APlayerController> OwningController;
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
    TMap<uint64,int32> PreviousPickupGrades;
    float HeroWalkPhase=0;
    FVector PreviousPlayer=FVector::ZeroVector;
    bool bHavePreviousPlayer=false;
    TArray<FTransient> Transients;
    TWeakObjectPtr<AActor> PreviousView;
    bool bActive=false, bSceneBuilt=false;
    float VisualTime=0;
    uint64 LastGeneration=0, LastShots=0, LastEffect=0;
    float LastHP=0, LastDash=0, SoundCooldown=0;
    int32 LastLevel=1;
    dywx::mvp::RunPhase LastPhase=dywx::mvp::RunPhase::Title;
    void BuildScene();
    void RebuildArena();
    static FName ElementBatch(dywx::mvp::Element Element, const TCHAR* Suffix);
    void LoadGeometry();
    void SetupVFX();
    void RenderVFX(const dywx::mvp::Snapshot& Snapshot,bool& NewPalm,bool& NewThunder,uint64& MaxEffect);
    void Render(float DeltaSeconds);
    void Add(FName Batch, const FVector& Position, const FVector& Scale=FVector::OneVector, float Yaw=0);
    void Flush();
    void MakeBatch(FName Name, FName MeshName, FLinearColor Tint, bool bDynamic=true, bool bEmissive=false);
    UStaticMesh* BuildMesh(FName Name, const TArray<FVector3f>& Vertices, const TArray<int32>& Indices, const TArray<FVector4f>& Colors, const TArray<FVector3f>& VertexNormals = {});
};
