#include "MVP/DywxMVPWorld.h"
#include <cmath>
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

using namespace dywx::mvp;
DEFINE_LOG_CATEGORY_STATIC(LogDywxMVPWorld, Log, All);
namespace {
FVector World(Vec2 P,float Z=0) { return FVector(P.x*100,-P.y*100,Z); }
float Heading(Vec2 P) { return FMath::RadiansToDegrees(FMath::Atan2(-P.y,P.x)); }
const FLinearColor Fire(1.f,.24f,.035f), Thunder(.48f,.22f,1.f), Danger(1.f,.025f,.055f);
bool AnomalyAffectsEnemy(const AnomalyState& State,const AnomalyModifiers& Mods,EnemyRole Role)
{
    if(!State.active) return false;
    switch(State.id) {
    case AnomalyId::SpiritTide: return Mods.currency>1.f || (Mods.spawn>1.f && Role!=EnemyRole::Boss && Role!=EnemyRole::SpiritBeast);
    case AnomalyId::Fissure: return Mods.chargers>1.f && Role==EnemyRole::Charger;
    case AnomalyId::Fog: return (Mods.ranged>1.f && Role==EnemyRole::Ranged) || (Mods.beasts>1.f && Role==EnemyRole::SpiritBeast);
    case AnomalyId::Eclipse: return Mods.xp>1.f || Mods.nightEnemyPower>1.f;
    // These anomalies affect terrain/the player, not enemy attributes.
    case AnomalyId::Lightning: case AnomalyId::Array: case AnomalyId::Count: return false;
    }
    return false;
}
FName AnomalyHalo(AnomalyId Id) { return FName(*FString::Printf(TEXT("AnomalyHalo%d"),int32(Id))); }
}
ADywxMVPWorld::ADywxMVPWorld()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Scene")));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->ProjectionMode=ECameraProjectionMode::Orthographic;
    Camera->OrthoWidth=3200;
    Camera->SetRelativeRotation(FRotator(-62,-90,0));
    Camera->bConstrainAspectRatio=false;
    Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;
    Camera->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    // The stylized scene uses authored relative light/emissive values, not lux
    // calibrated for the default physical camera (f/4, 1/60 s, ISO 100).
    Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    Camera->PostProcessSettings.bOverride_AutoExposureBias=true;
    Camera->PostProcessSettings.AutoExposureBias=0;
    Camera->PostProcessSettings.bOverride_BloomIntensity=true;
    Camera->PostProcessSettings.BloomIntensity=.3f;
    SetActorTickEnabled(false);
}
void ADywxMVPWorld::InitializePresentation(UDywxRunSession* InSession, APlayerController* InController)
{
    check(!bActive);
    Session = InSession;
    OwningController = InController;
}
void ADywxMVPWorld::ActivateRun()
{
    if (!Session || !Session->IsRunActive() || !OwningController.IsValid()) return;
    if(!bSceneBuilt) BuildScene();
    RebuildArena();
    SoundCooldown=0; LastHP=GetSnapshot().player.hp; LastShots=0; LastEffect=0; LastDash=0; LastLevel=1; LastPhase=RunPhase::Title; VisualTime=0; HeroWalkPhase=0; SwarmWalkPhases.Reset(); bHavePreviousPlayer=false; Transients.Reset(); PreviousEnemies.Reset(); PreviousPickups.Reset(); PreviousPickupGrades.Reset();
    LastGeneration=0;
    if(!bActive) {
        APlayerController* PC=OwningController.Get();
        PreviousView=PC->GetViewTarget(); PC->SetViewTarget(this);
    }
    bActive=true; SetActorHiddenInGame(false); SetActorTickEnabled(true); Render(0);
}
void ADywxMVPWorld::StopRun()
{
    if(!bActive) return;
    bActive=false; SetActorTickEnabled(false); SetActorHiddenInGame(true);
    if(APlayerController* PC=OwningController.Get()) if(PC->GetViewTarget()==this && PreviousView.IsValid()) PC->SetViewTarget(PreviousView.Get());
    PreviousView.Reset();
}
void ADywxMVPWorld::EndPlay(const EEndPlayReason::Type Reason)
{
    StopRun(); Session=nullptr; OwningController.Reset(); Super::EndPlay(Reason);
}
void ADywxMVPWorld::Tick(float Dt)
{
    Super::Tick(Dt);
    if(!bActive) return;
    if(!Session || !Session->IsRunActive()) { StopRun(); return; }
    const auto& S=GetSnapshot();
    const bool Frozen=S.paused || (S.phase!=RunPhase::Wave && S.phase!=RunPhase::Boss);
    const float VisualDt=Frozen?0.f:FMath::Clamp(Dt,0.f,.1f);
    VisualTime+=VisualDt;
    Render(VisualDt);
}
UStaticMesh* ADywxMVPWorld::BuildMesh(FName Name,const TArray<FVector3f>& V,const TArray<int32>& I,const TArray<FVector4f>& C,const TArray<FVector3f>& VertexNormals)
{
    if(V.IsEmpty() || I.Num()%3!=0) return nullptr;
    FMeshDescription D; FStaticMeshAttributes A(D); A.Register();
    auto Positions=A.GetVertexPositions(); auto Normals=A.GetVertexInstanceNormals();
    auto Tangents=A.GetVertexInstanceTangents(); auto Signs=A.GetVertexInstanceBinormalSigns();
    auto Colors=A.GetVertexInstanceColors(); auto UV=A.GetVertexInstanceUVs(); UV.SetNumChannels(1);
    const FPolygonGroupID Group=D.CreatePolygonGroup();
    A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Surface");
    TArray<FVertexID> IDs; IDs.Reserve(V.Num());
    for(const auto& P:V) { auto Id=D.CreateVertex(); Positions[Id]=P; IDs.Add(Id); }
    for(int32 T=0;T<I.Num();T+=3) {
        if(!V.IsValidIndex(I[T]) || !V.IsValidIndex(I[T+1]) || !V.IsValidIndex(I[T+2])) return nullptr;
        FVector3f N=FVector3f::CrossProduct(V[I[T+1]]-V[I[T]],V[I[T+2]]-V[I[T]]).GetSafeNormal();
        if(N.IsNearlyZero()) continue;
        FVector3f Tangent=FVector3f::CrossProduct(FMath::Abs(N.Z)<.9f?FVector3f(0,0,1):FVector3f(0,1,0),N).GetSafeNormal();
        TArray<FVertexInstanceID> Corners;
        for(int32 K=0;K<3;K++) { int32 Index=I[T+K]; auto Id=D.CreateVertexInstance(IDs[Index]); Corners.Add(Id); Normals[Id]=VertexNormals.IsValidIndex(Index)?VertexNormals[Index].GetSafeNormal():N; Tangents[Id]=Tangent; Signs[Id]=1; Colors[Id]=C.IsValidIndex(Index)?C[Index]:FVector4f(1,1,1,1); UV.Set(Id,0,FVector2f(0,0)); }
        D.CreatePolygon(Group,Corners);
    }
    if(D.Triangles().Num()==0) { UE_LOG(LogDywxMVPWorld,Error,TEXT("Rejected empty/degenerate mesh: %s"),*Name.ToString()); return nullptr; }
    UStaticMesh* Mesh=NewObject<UStaticMesh>(this,Name,RF_Transient);
    FStaticMaterial Surface(nullptr,TEXT("Surface"));
    // These vertex-colored meshes have zero UVs and no sampled textures. Fast
    // runtime builds skip editor UV-density generation; streaming still reads it.
    Surface.UVChannelData=FMeshUVChannelInfo(0.f);
    Mesh->GetStaticMaterials().Add(Surface);
    UStaticMesh::FBuildMeshDescriptionsParams Params; Params.bFastBuild=true; Params.bBuildSimpleCollision=false; Params.bAllowCpuAccess=false;
    TArray<const FMeshDescription*> Descriptions{&D};
    if(!Mesh->BuildFromMeshDescriptions(Descriptions,Params)) { UE_LOG(LogDywxMVPWorld,Error,TEXT("Runtime static mesh build failed: %s"),*Name.ToString()); return nullptr; }
    OwnedMeshes.Add(Mesh); Meshes.Add(Name,Mesh); return Mesh;
}
void ADywxMVPWorld::LoadGeometry()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(DywxWorld_LoadGeometry);
    FString Data; TSharedPtr<FJsonObject> Root;
    const FString Path=FPaths::ProjectContentDir()/TEXT("MVP/Geometry/scene.json");
    const int64 FileBytes=IFileManager::Get().FileSize(*Path);
    if(FileBytes>0 && FileBytes<=32*1024*1024 && FFileHelper::LoadFileToString(Data,*Path) && Data.Len()<=32*1024*1024 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),Root) && Root.IsValid()) {
        double Version=0;
        TSet<FString> Allowed{TEXT("hero_body"),TEXT("hero_sword"),TEXT("enemy_swarm"),TEXT("enemy_exploder"),TEXT("enemy_charger"),TEXT("enemy_rangedbird"),TEXT("enemy_supporttotem"),TEXT("enemy_summoner"),TEXT("enemy_boss"),TEXT("enemy_spirit"),TEXT("rock"),TEXT("pillar"),TEXT("tile"),TEXT("shrine_banner"),TEXT("shrine_arch")};
        for(int32 Pose=0;Pose<8;Pose++) { Allowed.Add(FString::Printf(TEXT("hero_walk_%d"),Pose)); Allowed.Add(FString::Printf(TEXT("enemy_swarm_walk_%d"),Pose)); }
        const TArray<TSharedPtr<FJsonValue>>* Entries=nullptr;
        if(Root->TryGetNumberField(TEXT("version"),Version) && Version==1 && Root->TryGetArrayField(TEXT("meshes"),Entries) && Entries->Num()<=64) for(const auto& Entry:*Entries) {
            if(!Entry.IsValid() || Entry->Type!=EJson::Object) continue;
            auto O=Entry->AsObject(); if(!O.IsValid()) continue;
            FString Name; const TArray<TSharedPtr<FJsonValue>> *Vertices=nullptr,*Triangles=nullptr,*Colors=nullptr,*Normals=nullptr;
            if(!O->TryGetStringField(TEXT("name"),Name) || !Allowed.Contains(Name) || Meshes.Contains(FName(*Name)) || !O->TryGetArrayField(TEXT("vertices"),Vertices) || !O->TryGetArrayField(TEXT("triangles"),Triangles) || !O->TryGetArrayField(TEXT("colors"),Colors) || Vertices->Num()==0 || Vertices->Num()>100000 || Triangles->Num()>100000 || Colors->Num()!=Vertices->Num()) continue;
            O->TryGetArrayField(TEXT("normals"),Normals);
            if(Normals && Normals->Num()!=Vertices->Num()) continue;
            TArray<FVector3f> V; TArray<int32> I; TArray<FVector4f> C; TArray<FVector3f> N; bool Valid=true;
            auto Numbers=[](const TSharedPtr<FJsonValue>& Value,int32 Count,double* Out,double Min,double Max) {
                if(!Value.IsValid() || Value->Type!=EJson::Array) return false;
                const auto& Array=Value->AsArray(); if(Array.Num()!=Count) return false;
                for(int32 K=0;K<Count;K++) if(!Array[K].IsValid() || !Array[K]->TryGetNumber(Out[K]) || !FMath::IsFinite(Out[K]) || Out[K]<Min || Out[K]>Max) return false;
                return true;
            };
            for(const auto& P:*Vertices) { double X[3]; if(!Numbers(P,3,X,-1000,1000)) {Valid=false;break;} V.Add(FVector3f(float(X[0]*100),float(-X[1]*100),float(X[2]*100))); }
            for(const auto& P:*Triangles) { double X[3]; if(!Numbers(P,3,X,0,Vertices->Num()-1) || X[0]!=std::floor(X[0]) || X[1]!=std::floor(X[1]) || X[2]!=std::floor(X[2])) {Valid=false;break;} I.Append({int32(X[0]),int32(X[2]),int32(X[1])}); }
            for(const auto& P:*Colors) { double X[4]; if(!Numbers(P,4,X,0,1)) {Valid=false;break;} C.Add(FVector4f(float(X[0]),float(X[1]),float(X[2]),float(X[3]))); }
            if(Normals) for(const auto& P:*Normals) { double X[3]; if(!Numbers(P,3,X,-1.001,1.001)) {Valid=false;break;} FVector3f Normal{float(X[0]),float(-X[1]),float(X[2])}; if(Normal.SizeSquared()<.001f) {Valid=false;break;} N.Add(Normal.GetSafeNormal()); }
            if(Valid) BuildMesh(FName(*Name),V,I,C,N);
            else UE_LOG(LogDywxMVPWorld,Error,TEXT("Rejected invalid geometry arrays: %s"),*Name);
        }
    }
    if(!Meshes.Contains(TEXT("hero_body"))) UE_LOG(LogDywxMVPWorld,Error,TEXT("Missing/invalid staged MVP geometry %s; procedural diagnostic silhouettes will be used. This is NOT the authored visual target."),*Path);
    // Small code-owned VFX geometry. No Engine/BasicShapes or editor-only asset dependency.
    TArray<FVector3f> Box{{-50,-50,0},{50,-50,0},{50,50,0},{-50,50,0},{-50,-50,100},{50,-50,100},{50,50,100},{-50,50,100}};
    TArray<int32> BI{0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,1,2,6,1,6,5,2,3,7,2,7,6,3,0,4,3,4,7}; BuildMesh(TEXT("unit_box"),Box,BI,{});
    for(int32 Ring=0;Ring<2;Ring++) {
        TArray<FVector3f> V; TArray<int32> I;
        for(int32 J=0;J<64;J++) { const float A=2*PI*J/64; V.Add(FVector3f(FMath::Cos(A)*100,FMath::Sin(A)*100,0)); V.Add(FVector3f(FMath::Cos(A)*(Ring?90:0),FMath::Sin(A)*(Ring?90:0),0)); }
        for(int32 J=0;J<64;J++) {int32 K=(J+1)%64; I.Append({J*2,K*2,J*2+1,K*2,K*2+1,J*2+1});}
        BuildMesh(Ring?TEXT("unit_ring"):TEXT("unit_disc"),V,I,{});
    }
    TArray<FVector3f> Diamond{{0,0,70},{40,0,0},{0,40,0},{-40,0,0},{0,-40,0},{0,0,-30}};
    TArray<int32> DI{0,1,2,0,2,3,0,3,4,0,4,1,5,2,1,5,3,2,5,4,3,5,1,4}; BuildMesh(TEXT("unit_diamond"),Diamond,DI,{});
}
void ADywxMVPWorld::MakeBatch(FName Name,FName MeshName,FLinearColor Tint,bool Dynamic,bool Emissive)
{
    UStaticMesh* Mesh=Meshes.FindRef(MeshName); if(!Mesh) { UE_LOG(LogDywxMVPWorld,Error,TEXT("Missing authored MVP mesh %s: diagnostic silhouette only, not visual acceptance."),*MeshName.ToString()); Mesh=Meshes.FindRef(TEXT("unit_diamond")); }
    // Logical batch keys can equal mesh keys (Rock/rock, Pillar/pillar).
    // UObject names share one case-insensitive namespace within this actor.
    // Keep Name in Batches, but let Unreal allocate a unique component name.
    auto* Component=NewObject<UInstancedStaticMeshComponent>(this,NAME_None,RF_Transient);
    Component->SetupAttachment(RootComponent); Component->SetMobility(EComponentMobility::Movable);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); Component->SetGenerateOverlapEvents(false);
    Component->SetCastShadow(false); Component->bReceivesDecals=false; Component->SetStaticMesh(Mesh);
    const TCHAR* Path=Emissive?TEXT("/Game/MVP/M_VFX.M_VFX"):TEXT("/Game/MVP/M_Surface.M_Surface");
    UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,Path);
    if(!Base) { Base=UMaterial::GetDefaultMaterial(MD_Surface); UE_LOG(LogDywxMVPWorld,Error,TEXT("Required %s missing. Run scripts/bootstrap-mvp-assets.py with UnrealEditor before playing/cooking. Default material is diagnostic only."),Path); }
    auto* Material=UMaterialInstanceDynamic::Create(Base,this); Material->SetVectorParameterValue(TEXT("Tint"),Tint);
    Material->SetScalarParameterValue(TEXT("Glow"),Emissive?1.8f:0.f);
    Component->SetMaterial(0,Material); AddInstanceComponent(Component); Component->RegisterComponent();
    OwnedMaterials.Add(Material); OwnedBatches.Add(Component);
    FBatch Batch; Batch.Component=Component; Batch.Dynamic=Dynamic; Batches.Add(Name,MoveTemp(Batch));
}
void ADywxMVPWorld::Add(FName Name,const FVector& P,const FVector& Scale,float Yaw)
{
    if(auto* B=Batches.Find(Name)) B->Pending.Emplace(FRotator(0,Yaw,0),P,Scale);
}
void ADywxMVPWorld::Flush()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(DywxWorld_Flush);
    for(auto& Pair:Batches) {
        auto& B=Pair.Value; if(!B.Dynamic && B.Pending.IsEmpty()) continue;
        const int32 Count=B.Pending.Num();
        // Pose occupancy moves between eight buckets. Bound retained slack rather than
        // keeping eight historical peaks (no duplicate visible actor or per-frame mesh build).
        if(Pair.Key.ToString().StartsWith(TEXT("Walk_")) || Pair.Key==FName(TEXT("Enemy_swarm")) || Pair.Key==FName(TEXT("HeroBody")))
            while(B.Allocated>Count+8) { B.Component->RemoveInstance(B.Allocated-1); --B.Allocated; }
        while(B.Allocated<Count) { B.Component->AddInstance(FTransform(FVector(0,0,-100000))); B.Allocated++; }
        for(int32 I=0;I<B.Allocated;I++) {
            const FTransform T=I<Count?B.Pending[I]:FTransform(FQuat::Identity,FVector(0,0,-100000),FVector::ZeroVector);
            B.Component->UpdateInstanceTransform(I,T,false,I==B.Allocated-1,true);
        }
        B.Pending.Reset();
    }
}
void ADywxMVPWorld::BuildScene()
{
    LoadGeometry();
    SetupVFX();
    SoundConcurrency=NewObject<USoundConcurrency>(this);
    SoundConcurrency->Concurrency.MaxCount=4;
    SoundConcurrency->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::PreventNew;
    for(const TCHAR* Cue:{TEXT("palm"),TEXT("sword"),TEXT("thunder"),TEXT("hit"),TEXT("pickup"),TEXT("dash"),TEXT("upgrade"),TEXT("boss"),TEXT("win"),TEXT("loss")}) {
        const FString Path=FString::Printf(TEXT("/Game/MVP/Audio/%s.%s"),Cue,Cue);
        if(auto* Sound=LoadObject<USoundBase>(nullptr,*Path)) Sounds.Add(FName(Cue),Sound);
        else UE_LOG(LogDywxMVPWorld,Error,TEXT("Missing required MVP cue %s. Run material/audio bootstrap before packaging."),*Path);
    }
    MakeBatch(TEXT("Ground"),TEXT("unit_box"),FLinearColor(.025,.04,.065),false);
    MakeBatch(TEXT("Tiles"),TEXT("tile"),FLinearColor(.66,.65,.64),false);
    MakeBatch(TEXT("Rock"),TEXT("rock"),FLinearColor(.7,.8,1),false);
    MakeBatch(TEXT("Pillar"),TEXT("pillar"),FLinearColor(.8,.85,1),false);
    if(Meshes.Contains(TEXT("shrine_banner"))) MakeBatch(TEXT("Banner"),TEXT("shrine_banner"),FLinearColor::White,false);
    if(Meshes.Contains(TEXT("shrine_arch"))) MakeBatch(TEXT("Arch"),TEXT("shrine_arch"),FLinearColor::White,false);
    MakeBatch(TEXT("Edge"),TEXT("unit_box"),FLinearColor(.11,.23,.35),false,true);
    MakeBatch(TEXT("FloorRune"),TEXT("unit_ring"),FLinearColor(.06,.12,.2),false,true);
    MakeBatch(TEXT("HeroBody"),TEXT("hero_body"),FLinearColor::White);
    for(int32 Pose=0;Pose<8;Pose++) {
        const FName HeroMesh(*FString::Printf(TEXT("hero_walk_%d"),Pose));
        const FName SwarmMesh(*FString::Printf(TEXT("enemy_swarm_walk_%d"),Pose));
        if(Meshes.Contains(HeroMesh)) MakeBatch(FName(*FString::Printf(TEXT("Walk_Hero_%d"),Pose)),HeroMesh,FLinearColor::White);
        if(Meshes.Contains(SwarmMesh)) MakeBatch(FName(*FString::Printf(TEXT("Walk_Swarm_%d"),Pose)),SwarmMesh,FLinearColor::White);
    }
    MakeBatch(TEXT("HeroSword"),TEXT("hero_sword"),FLinearColor::White);
    const TCHAR* Roles[]={TEXT("swarm"),TEXT("exploder"),TEXT("charger"),TEXT("rangedbird"),TEXT("supporttotem"),TEXT("summoner"),TEXT("boss"),TEXT("spirit")};
    for(auto EnemyRoleName:Roles) MakeBatch(FName(*FString::Printf(TEXT("Enemy_%s"),EnemyRoleName)),FName(*FString::Printf(TEXT("enemy_%s"),EnemyRoleName)),FLinearColor::White);
    MakeBatch(TEXT("Shadow"),TEXT("unit_disc"),FLinearColor(.008,.012,.02));
    MakeBatch(TEXT("PlayerRing"),TEXT("unit_ring"),FLinearColor(.35,.25,.07),true,true);
    MakeBatch(TEXT("Invulnerable"),TEXT("unit_ring"),FLinearColor(.4,.9,1),true,true);
    MakeBatch(TEXT("EnemyHealth"),TEXT("unit_box"),FLinearColor(.7,.065,.08),true,true);
    MakeBatch(TEXT("EnemyHealthBack"),TEXT("unit_box"),FLinearColor(.025,.025,.035));
    MakeBatch(TEXT("WarningRing"),TEXT("unit_ring"),Danger,true,true);
    MakeBatch(TEXT("WarningBeam"),TEXT("unit_box"),Danger,true,true);
    MakeBatch(TEXT("FireRing"),TEXT("unit_ring"),Fire,true,true);
    MakeBatch(TEXT("ThunderRing"),TEXT("unit_ring"),Thunder,true,true);
    MakeBatch(TEXT("FireBeam"),TEXT("unit_box"),Fire,true,true);
    MakeBatch(TEXT("ThunderBeam"),TEXT("unit_box"),Thunder,true,true);
    MakeBatch(TEXT("FireShot"),TEXT("hero_sword"),Fire,true,true);
    MakeBatch(TEXT("ThunderShot"),TEXT("hero_sword"),Thunder,true,true);
    MakeBatch(TEXT("EnemyShot"),TEXT("unit_diamond"),Danger,true,true);
    MakeBatch(TEXT("XP"),TEXT("unit_diamond"),FLinearColor(.08,.8,.65),true,true);
    MakeBatch(TEXT("Relic"),TEXT("unit_diamond"),FLinearColor(1,.65,.13),true,true);
    MakeBatch(TEXT("RelicBeam"),TEXT("unit_box"),FLinearColor(.75,.35,1),true,true);
    MakeBatch(TEXT("Vein"),TEXT("unit_ring"),FLinearColor(.08,.45,.4),false,true);
    MakeBatch(TEXT("VeinCrystal"),TEXT("unit_diamond"),FLinearColor(.1,.7,.6),false,true);
    MakeBatch(TEXT("ArrayPlatform"),TEXT("unit_ring"),FLinearColor(.2,.18,.3),false,true);
    MakeBatch(TEXT("BeastCompanion"),TEXT("enemy_spirit"),FLinearColor(.3,1,.65));
    MakeBatch(TEXT("BeastHalo"),TEXT("unit_ring"),FLinearColor(.1,.7,.38),true,true);
    const FLinearColor Grades[]={FLinearColor(.75,.8,.85),FLinearColor(.1,.7,1),FLinearColor(.65,.2,1),FLinearColor(1,.65,.08)};
    for(int32 R=0;R<4;R++) MakeBatch(FName(*FString::Printf(TEXT("RelicGrade%d"),R)),TEXT("unit_diamond"),Grades[R],true,true);
    MakeBatch(TEXT("Death"),TEXT("unit_ring"),FLinearColor(.9,.35,.1),true,true);
    MakeBatch(TEXT("Collect"),TEXT("unit_ring"),FLinearColor(.15,1,.8),true,true);
    auto* Sun=NewObject<UDirectionalLightComponent>(this,TEXT("MoonLight")); Sun->SetupAttachment(RootComponent); AddInstanceComponent(Sun);
    Sun->SetMobility(EComponentMobility::Movable); Sun->SetRelativeRotation(FRotator(-55,-40,0)); Sun->SetIntensity(3.f); Sun->SetLightColor(FLinearColor(.63,.75,1)); Sun->SetCastShadows(false); Sun->RegisterComponent();
    auto* Fill=NewObject<UDirectionalLightComponent>(this,TEXT("WarmRim")); Fill->SetupAttachment(RootComponent); AddInstanceComponent(Fill);
    Fill->SetMobility(EComponentMobility::Movable); Fill->SetRelativeRotation(FRotator(-25,140,0)); Fill->SetIntensity(1.5f); Fill->SetLightColor(FLinearColor(1,.68,.38)); Fill->SetCastShadows(false); Fill->RegisterComponent();
    Flush(); bSceneBuilt=true;
    UE_LOG(LogDywxMVPWorld,Display,TEXT("Runtime scene ready: meshes=%d batches=%d"),Meshes.Num(),Batches.Num());
}
void ADywxMVPWorld::RebuildArena()
{
    // Run-boundary only: reuse meshes/materials, clear all old map instances.
    for(auto& Pair:Batches) if(!Pair.Value.Dynamic) {
        Pair.Value.Component->ClearInstances(); Pair.Value.Allocated=0; Pair.Value.Pending.Reset();
    }
    const auto& Simulation=*Session->GetSimulation();
    const auto& Arena=Simulation.GetArena();
    const bool Full=Simulation.IsCampaign();
    const float X=Full?Arena.config.halfWidth:48, Y=Full?Arena.config.halfHeight:27;
    const float Cut=Full?Arena.config.cornerCut:0;
    const int32 Cave=Full?int32(Arena.cave):0;
    const FLinearColor Palettes[]={FLinearColor(.34,.35,.38),FLinearColor(.39,.31,.28),FLinearColor(.3,.36,.32)};
    for(const TCHAR* Key:{TEXT("Ground"),TEXT("Rock"),TEXT("Pillar"),TEXT("Edge")}) if(auto* Batch=Batches.Find(Key))
        if(auto* Mat=Cast<UMaterialInstanceDynamic>(Batch->Component->GetMaterial(0)))
            Mat->SetVectorParameterValue(TEXT("Tint"),Palettes[FMath::Clamp(Cave,0,2)]*(FString(Key)==TEXT("Ground")?.12f:1.f));
    Add(TEXT("Ground"),FVector(0,0,-45),FVector((X+4)*2,(Y+4)*2,.4));
    for(int TX=int(-X+2);TX<X-1;TX+=4) for(int TY=int(-Y+2);TY<Y-1;TY+=4)
        if(!Full || Arena.Contains({float(TX),float(TY)},1.5f)) Add(TEXT("Tiles"),World({float(TX),float(TY)},-4),FVector(4,4,1),((TX+TY)%3)*90);
    if(Full) { for(const auto& O:Arena.obstacles) Add(TEXT("Rock"),World(O.position),FVector(O.radius),O.position.x*17); }
    else { for(const auto& O:GetArenaObstacles()) Add(TEXT("Rock"),World(O.position),FVector(O.radius),O.position.x*17); }
    const Vec2 Corners[]={{-X+Cut,-Y},{X-Cut,-Y},{X,-Y+Cut},{X,Y-Cut},{X-Cut,Y},{-X+Cut,Y},{-X,Y-Cut},{-X,-Y+Cut}};
    for(int32 I=0;I<8;I++) {
        const FVector A=World(Corners[I],3),B=World(Corners[(I+1)%8],3),D=B-A;
        if(D.SizeSquared()>1) Add(TEXT("Edge"),(A+B)*.5f,FVector(D.Size()/100,.15,.08),D.Rotation().Yaw);
    }
    if(Full) {
        for(auto P:Arena.veins) { Add(TEXT("Vein"),World(P,2),FVector(1.3,1.3,1)); Add(TEXT("VeinCrystal"),World(P,12),FVector(.24)); }
        for(auto P:Arena.hazards) Add(TEXT("ArrayPlatform"),World(P,2),FVector(2.2,2.2,1));
    }
    for(int TX=int(-X+4);TX<X;TX+=8) for(float Sign:{-1.f,1.f}) Add(TEXT("Pillar"),World({float(TX),Sign*(Y+2)}),FVector(1.1));
    Add(TEXT("Arch"),World({0,Y+3}),FVector(1.5),-90);
    for(float TX:{-12.f,12.f}) Add(TEXT("Banner"),World({TX,Y+2}),FVector(1.5),-90);
    Add(TEXT("FloorRune"),FVector(0,0,1),FVector(8,8,1));
    Flush();
}
FName ADywxMVPWorld::ElementBatch(Element E,const TCHAR* Suffix)
{
    const TCHAR* Prefix=TEXT("Neutral");
    switch(E) {
    case Element::Fire: Prefix=TEXT("Fire"); break;
    case Element::Thunder: Prefix=TEXT("Thunder"); break;
    case Element::Metal: Prefix=TEXT("Metal"); break;
    case Element::Wood: Prefix=TEXT("Wood"); break;
    case Element::Water: Prefix=TEXT("Water"); break;
    case Element::Earth: Prefix=TEXT("Earth"); break;
    default: break;
    }
    return FName(*(FString(Prefix)+Suffix));
}
void ADywxMVPWorld::Render(float Dt)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(DywxWorld_Render);
    const auto& Simulation=*Session->GetSimulation();
    const auto& S=GetSnapshot();
    if(S.generation!=LastGeneration) { PreviousEnemies.Reset(); PreviousPickups.Reset(); Transients.Reset(); PreviousPickupGrades.Reset(); LastEffect=0; LastShots=S.statistics.shots; LastHP=S.player.hp; LastDash=S.player.dashCooldown; LastLevel=S.progression.level; LastPhase=RunPhase::Title; LastGeneration=S.generation; bHavePreviousPlayer=false; HeroWalkPhase=0; SwarmWalkPhases.Reset(); }
    FVector Player=World(S.player.position);
    // Aspect-adaptive ground footprint. HUD overlays do not move this authoritative view.
    int32 Width=1920,Height=1080; if(auto* PC=OwningController.Get()) PC->GetViewportSize(Width,Height);
    const float Aspect=float(FMath::Max(Width,1))/FMath::Max(Height,1);
    Camera->OrthoWidth=FMath::Max(3200.f,1800.f*Aspect);
    const float HalfX=FMath::Min(4990.f,Camera->OrthoWidth*.5f);
    const float HalfY=FMath::Min(2890.f,Camera->OrthoWidth/Aspect*.5f/FMath::Sin(FMath::DegreesToRadians(62.f)));
    // Two-metre decorative border keeps tall sprites visible at collision bounds.
    const FVector Focus(FMath::Clamp(float(Player.X),-5000+HalfX,5000-HalfX),FMath::Clamp(float(Player.Y),-2900+HalfY,2900-HalfY),0);
    Camera->SetWorldLocation(Focus-Camera->GetForwardVector()*4500);
    Add(TEXT("Shadow"),Player+FVector(0,0,2),FVector(.55,.38,1));
    // Authoritative displacement, not held input: walls/idle/pause cannot walk in place.
    const float HeroDistance=bHavePreviousPlayer?float(FVector::Dist2D(Player,PreviousPlayer)):0.f;
    const bool HeroMoving=Dt>0 && HeroDistance>.1f;
    // World positions are centimetres: one hero cycle per 1.5 simulation units.
    if(HeroMoving) HeroWalkPhase=FMath::Fmod(HeroWalkPhase+HeroDistance/150.f,1.f);
    PreviousPlayer=Player; bHavePreviousPlayer=true;
    const float Bob=HeroMoving?FMath::Abs(FMath::Sin(HeroWalkPhase*2*PI))*2:0;
    FName HeroBatch=TEXT("HeroBody");
    if(HeroMoving) { const FName Pose(*FString::Printf(TEXT("Walk_Hero_%d"),FMath::FloorToInt(HeroWalkPhase*8)&7)); if(Batches.Contains(Pose)) HeroBatch=Pose; }
    Add(HeroBatch,Player+FVector(0,0,Bob),FVector::OneVector,Heading(S.player.facing));
    if(S.progression.build.hero==HeroId::Sword) Add(TEXT("HeroSword"),Player+FVector(45,0,125+Bob),FVector(.8),Heading(S.player.facing));
    if(S.progression.build.hero==HeroId::Beast) {
        // Existing spirit mesh, deliberately recognizable as a companion, not new character art.
        const FVector Companion=Player+FVector(FMath::Cos(VisualTime)*90,FMath::Sin(VisualTime)*90,50);
        Add(TEXT("BeastCompanion"),Companion,FVector(.45),Heading(S.player.facing));
        Add(TEXT("BeastHalo"),Player+FVector(0,0,5),FVector(.75,.75,1));
    }
    if(Simulation.IsCampaign()) {
        const auto& A=Simulation.GetAnomalies().State();
        const auto Mods=Simulation.GetAnomalies().Modifiers(S.player.position);
        if(A.active && A.id==AnomalyId::Array) Add(AnomalyHalo(AnomalyId::Array),World(A.anchor,3),FVector(Simulation.GetAnomalies().Config().arrayRadius,Simulation.GetAnomalies().Config().arrayRadius,1));
        if(Mods.playerPower>1.f) Add(AnomalyHalo(AnomalyId::Array),Player+FVector(0,0,3),FVector(.95,.95,1));
        if(Mods.thunderPower>1.f) Add(AnomalyHalo(AnomalyId::Lightning),Player+FVector(0,0,3),FVector(1.05,1.05,1));
    }
    Add(TEXT("PlayerRing"),Player+FVector(0,0,4),FVector(.65,.65,1));
    if(S.player.invulnerable>0 || S.player.shield>0) Add(TEXT("Invulnerable"),Player+FVector(0,0,8),FVector(.85,.85,1));
    TMap<uint64,FVector> CurrentEnemies,CurrentPickups;
    TMap<uint64,int32> CurrentPickupGrades;
    const float Vision=Simulation.IsCampaign()?Simulation.GetAnomalies().Modifiers(S.player.position).visionRange:1000.f;
    if(Vision<1000.f) Add(TEXT("WaterRing"),Player+FVector(0,0,3),FVector(Vision,Vision,1));
    CurrentEnemies.Reserve(int32(S.enemies.size())); CurrentPickups.Reserve(int32(S.pickups.size()));
    const TCHAR* Roles[]={TEXT("Enemy_swarm"),TEXT("Enemy_exploder"),TEXT("Enemy_charger"),TEXT("Enemy_rangedbird"),TEXT("Enemy_supporttotem"),TEXT("Enemy_summoner"),TEXT("Enemy_boss"),TEXT("Enemy_spirit")};
    for(const auto& E:S.enemies) {
        const FVector P=World(E.position); CurrentEnemies.Add(E.id,P);
        if(FVector::DistSquared2D(P,Player)>FMath::Square(Vision*100)) continue;
        if(Simulation.IsCampaign()) {
            const auto& Anomaly=Simulation.GetAnomalies();
            if(AnomalyAffectsEnemy(Anomaly.State(),Anomaly.Modifiers(S.player.position),E.role)) {
                // One thin, dim instance per affected visible enemy; never a light.
                // The red hostile telegraph is always above this Z=3 ground layer.
                const float Radius=E.radius+.12f;
                Add(AnomalyHalo(Anomaly.State().id),P+FVector(0,0,3),FVector(Radius,Radius,1));
            }
        }
        const float Scale=E.tier==EnemyTier::Elite?1.55f:1.f;
        FName EnemyBatch(Roles[FMath::Clamp(int(E.role),0,7)]);
        const FVector* Previous=PreviousEnemies.Find(E.id);
        if(int(E.role)==0) {
            // Stable spawn phase, then only actual travel advances gait. Elite stride scales
            // with its mesh. This map has one entry per live swarm, never historic IDs.
            float* ExistingPhase=SwarmWalkPhases.Find(E.id);
            if(!ExistingPhase) ExistingPhase=&SwarmWalkPhases.Add(E.id,float(E.id%8)/8.f);
            const float Distance=Previous?float(FVector::Dist2D(P,*Previous)):0.f;
            if(Dt>0 && Distance>.1f) {
                *ExistingPhase=FMath::Fmod(*ExistingPhase+Distance/(110.f*Scale),1.f);
                const FName Pose(*FString::Printf(TEXT("Walk_Swarm_%d"),FMath::FloorToInt(*ExistingPhase*8)&7));
                if(Batches.Contains(Pose)) EnemyBatch=Pose;
            }
        }
        Add(EnemyBatch,P+FVector(0,0,E.role==EnemyRole::Ranged?12*FMath::Sin(VisualTime*4+E.id):0),FVector(Scale),Heading(E.direction));
        Add(TEXT("Shadow"),P+FVector(0,0,2),FVector(E.radius,E.radius*.65,1));
        if(E.hp<E.maxHp || E.tier==EnemyTier::Boss || E.tier==EnemyTier::Elite) {
            const float BarWidth=E.tier==EnemyTier::Boss?2.8f:1.1f; const float Fraction=FMath::Clamp(E.hp/FMath::Max(E.maxHp,1.f),0.f,1.f);
            const FVector Bar=P+FVector(0,0,E.tier==EnemyTier::Boss?300:160*Scale);
            Add(TEXT("EnemyHealthBack"),Bar,FVector(BarWidth,.09,.04));
            Add(TEXT("EnemyHealth"),Bar+FVector(-(1-Fraction)*BarWidth*50,0,2),FVector(BarWidth*Fraction,.07,.04));
        }
        if(E.shield>0) Add(TEXT("ThunderRing"),P+FVector(0,0,10),FVector(E.radius*1.15,E.radius*1.15,1));
        if(E.mutations&8) for(int32 K=0;K<3;K++) {
            const float Angle=S.statistics.elapsed*2+K*2*PI/3;
            const Vec2 Orb=E.position+Vec2{FMath::Cos(Angle),FMath::Sin(Angle)}*1.7f;
            Add(TEXT("EnemyShot"),World(Orb,65),FVector(.3));
            Add(TEXT("WarningRing"),World(Orb,5),FVector(.65,.65,1));
        }
        // Burn ground marks are coalesced and budgeted in RenderVFX, not repeated per enemy.

    }
    for(const auto& P:S.projectiles) {
        const FVector At=World(P.position,65);
        Add(P.hostile?TEXT("EnemyShot"):ElementBatch(P.element,TEXT("Shot")),At,FVector(P.radius*1.8f,P.radius*.7f,P.radius*.7f),Heading(P.velocity));
        const FVector Before=World(P.previous,65); const FVector D=At-Before;
        if(!P.hostile && D.SizeSquared()>1) Add(ElementBatch(P.element,TEXT("Beam")),(At+Before)*.5f,FVector(D.Size()/100,.055,.035),D.Rotation().Yaw);
    }
    bool NewPalm=false, NewThunder=false; uint64 MaxEffect=LastEffect;
    RenderVFX(S,NewPalm,NewThunder,MaxEffect);
    for(const auto& P:S.pickups) {
        FVector At=World(P.position,25+FMath::Sin(VisualTime*3+P.id)*8); CurrentPickups.Add(P.id,At);
        if(P.hasRelic) CurrentPickupGrades.Add(P.id,int32(P.relic.rarity));
        Add(P.hasRelic?FName(*FString::Printf(TEXT("RelicGrade%d"),FMath::Clamp(int32(P.relic.rarity),0,3))):FName(TEXT("XP")),At,FVector(P.hasRelic?.42:.18),VisualTime*65);
        if(P.hasRelic && P.relic.rarity>=Rarity::Immortal) Add(TEXT("RelicBeam"),World(P.position),FVector(.05,.05,3));
    }
    // IDs are compared within one run generation; never attach death/collect effects to recycled indices.
    for(auto& E:PreviousEnemies) if(!CurrentEnemies.Contains(E.Key) && Transients.Num()<96) Transients.Add({E.Value,0,false});
    for(auto& P:PreviousPickups) if(!CurrentPickups.Contains(P.Key) && Transients.Num()<96) Transients.Add({P.Value,0,true});
    // At most one short voice per 120 ms; critical transition cues take precedence.
    int32 CollectedGrade=-1;
    for(const auto& P:PreviousPickupGrades) if(!CurrentPickups.Contains(P.Key)) CollectedGrade=FMath::Max(CollectedGrade,P.Value);
    SoundCooldown=FMath::Max(0.f,SoundCooldown-Dt); FName Cue=NAME_None;
    if(S.phase!=LastPhase) {
        if(S.phase==RunPhase::Won) Cue=TEXT("win");
        else if(S.phase==RunPhase::Dead) Cue=TEXT("loss");
        else if(S.phase==RunPhase::Boss) Cue=TEXT("boss");
    }
    if(Cue.IsNone() && SoundCooldown<=0) {
        if(CollectedGrade>=int32(Rarity::Immortal)) Cue=TEXT("upgrade");
        else if(S.progression.level>LastLevel) Cue=TEXT("upgrade");
        else if(S.player.hp<LastHP-.1f) Cue=TEXT("hit");
        else if(S.player.dashCooldown>LastDash+.5f) Cue=TEXT("dash");
        else if(CurrentPickups.Num()<PreviousPickups.Num()) Cue=TEXT("pickup");
        else if(NewThunder) Cue=TEXT("thunder");
        else if(NewPalm) Cue=TEXT("palm");
        else if(S.statistics.shots>LastShots) Cue=S.progression.build.hero==HeroId::Body?TEXT("palm"):TEXT("sword");
    }
    if(!Cue.IsNone()) { if(auto* Sound=Sounds.Find(Cue)) UGameplayStatics::PlaySound2D(this,Sound->Get(),.32f,CollectedGrade>=0?(.85f+.15f*CollectedGrade):1.f,0.f,SoundConcurrency,this); SoundCooldown=.12f; }
    LastEffect=MaxEffect; LastHP=S.player.hp; LastDash=S.player.dashCooldown; LastShots=S.statistics.shots; LastLevel=S.progression.level; LastPhase=S.phase;
    for(auto It=SwarmWalkPhases.CreateIterator();It;++It) if(!CurrentEnemies.Contains(It.Key())) It.RemoveCurrent();
    PreviousEnemies=MoveTemp(CurrentEnemies); PreviousPickups=MoveTemp(CurrentPickups); PreviousPickupGrades=MoveTemp(CurrentPickupGrades);
    for(int32 I=Transients.Num()-1;I>=0;I--) {
        auto& T=Transients[I]; T.Age+=Dt;
        if(T.Age>.35f) {Transients.RemoveAtSwap(I);continue;}
        const float Scale=.15f+T.Age*2.5f;
        Add(T.Pickup?TEXT("Collect"):TEXT("Death"),T.Position+FVector(0,0,14),FVector(Scale,Scale,1));
    }
    Flush();
}
