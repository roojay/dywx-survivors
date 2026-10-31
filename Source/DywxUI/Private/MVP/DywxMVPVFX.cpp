#include "MVP/DywxMVPWorld.h"

using namespace dywx::mvp;
namespace {
FVector World(Vec2 P,float Z=0) { return FVector(P.x*100,-P.y*100,Z); }
float Heading(Vec2 P) { return FMath::RadiansToDegrees(FMath::Atan2(-P.y,P.x)); }
}

void ADywxMVPWorld::SetupVFX()
{
    const Element Elements[]={Element::Metal,Element::Wood,Element::Water,Element::Earth,Element::None};
    const FLinearColor Colors[]={FLinearColor(.8,.8,.55),FLinearColor(.1,.65,.25),FLinearColor(.08,.55,.85),FLinearColor(.55,.35,.1),FLinearColor(.5,.5,.55)};
    for(int32 I=0;I<5;I++) {
        MakeBatch(ElementBatch(Elements[I],TEXT("Ring")),TEXT("unit_ring"),Colors[I],true,true);
        MakeBatch(ElementBatch(Elements[I],TEXT("Beam")),TEXT("unit_box"),Colors[I],true,true);
        MakeBatch(ElementBatch(Elements[I],TEXT("Shot")),TEXT("unit_diamond"),Colors[I],true,true);
    }
    // Startup-only geometry: sparse ember strokes, no opaque ground discs or transparency sorting.
    TArray<FVector3f> V; TArray<int32> I;
    for(int32 J=0;J<3;J++) {
        const float A=2*PI*J/3;
        const FVector3f P(FMath::Cos(A)*35,FMath::Sin(A)*35,0);
        const int32 N=V.Num(); V.Append({P+FVector3f(-18,-3,0),P+FVector3f(18,-3,0),P+FVector3f(8,3,0)}); I.Append({N,N+1,N+2});
    }
    BuildMesh(TEXT("unit_embers"),V,I,{});
    V.Reset(); I.Reset();
    for(int32 J=0;J<64;J++) { const float A=2*PI*J/64; for(float R:{100.f,98.5f}) V.Add(FVector3f(FMath::Cos(A)*R,FMath::Sin(A)*R,0)); }
    for(int32 J=0;J<64;J++) {const int32 K=(J+1)%64;I.Append({J*2,K*2,J*2+1,K*2,K*2+1,J*2+1});}
    BuildMesh(TEXT("unit_faint_boundary"),V,I,{});
    // Distinct but subdued anomaly colors; hostile red is reserved for danger.
    const FLinearColor AnomalyColors[]={FLinearColor(.18,.11,.32),FLinearColor(.04,.24,.19),FLinearColor(.27,.15,.04),FLinearColor(.08,.19,.24),FLinearColor(.22,.08,.22),FLinearColor(.20,.22,.08)};
    for(int32 K=0;K<int32(AnomalyId::Count);K++)
        MakeBatch(FName(*FString::Printf(TEXT("AnomalyHalo%d"),K)),TEXT("unit_faint_boundary"),AnomalyColors[K],true,true);
    const Element AuraElements[]={Element::Fire,Element::Thunder,Element::Metal,Element::Wood,Element::Water,Element::Earth,Element::None};
    const FLinearColor AuraColors[]={FLinearColor(.24,.07,.02),FLinearColor(.12,.055,.24),FLinearColor(.19,.19,.13),FLinearColor(.025,.16,.06),FLinearColor(.02,.14,.21),FLinearColor(.14,.09,.025),FLinearColor(.12,.12,.13)};
    for(int32 K=0;K<7;K++) MakeBatch(ElementBatch(AuraElements[K],TEXT("Aura")),TEXT("unit_faint_boundary"),AuraColors[K],true,true);
    MakeBatch(TEXT("BurnEmbers"),TEXT("unit_embers"),FLinearColor(.22,.055,.008),true,true);
    MakeBatch(TEXT("AuraBoundary"),TEXT("unit_faint_boundary"),FLinearColor(.16,.075,.025),true,true);
    MakeBatch(TEXT("ThunderBoundary"),TEXT("unit_faint_boundary"),FLinearColor(.12,.055,.24),true,true);
    MakeBatch(TEXT("FriendlyBoundary"),TEXT("unit_faint_boundary"),FLinearColor(.24,.09,.02),true,true);
}

void ADywxMVPWorld::RenderVFX(const Snapshot& S,bool& NewPalm,bool& NewThunder,uint64& MaxEffect)
{
    // Only noncritical friendly presentation is budgeted. Hostile telegraphs bypass it.
    constexpr int32 FriendlyInstanceBudget=96;
    int32 FriendlyInstances=0;
    TSet<FIntPoint> BurnCells;
    // Co-located large friendly areas share one boundary, even when a proc is not followPlayer.
    auto IsAura=[&](const EffectState& E) {
        const float DX=E.position.x-S.player.position.x,DY=E.position.y-S.player.position.y;
        return !E.hostile && E.form!=Form::Beast && E.kind!=EffectKind::Slash &&
            (E.followPlayer || (E.kind==EffectKind::Burn && E.radius>1.f && DX*DX+DY*DY<.64f));
    };
    std::array<float,static_cast<std::size_t>(Element::Count)> AuraRadii{};
    for(const auto& E:S.effects) if(IsAura(E)) {
        const int32 K=FMath::Clamp(int32(E.element),0,int32(Element::Count)-1);
        AuraRadii[K]=FMath::Max(AuraRadii[K],E.radius);
    }
    for(int32 K=0;K<int32(Element::Count);K++) if(AuraRadii[K]>0) {
        Add(ElementBatch(Element(K),TEXT("Aura")),World(S.player.position,3),FVector(AuraRadii[K],AuraRadii[K],1)); ++FriendlyInstances;
    }
    for(const auto& E:S.effects) {
        if(E.id>LastEffect) { NewPalm |= E.source==SpellId::FlamePalm; NewThunder |= !E.hostile && E.element==Element::Thunder; }
        MaxEffect=FMath::Max(MaxEffect,uint64(E.id));
        auto Emit=[&](FName Batch,const FVector& P,const FVector& Scale,float Yaw=0.f) {
            if(E.hostile || FriendlyInstances<FriendlyInstanceBudget) {
                Add(Batch,P,Scale,Yaw); if(!E.hostile) ++FriendlyInstances;
            }
        };
        const bool Warning=E.warning>0; const FName Ring=E.hostile?TEXT("WarningRing"):ElementBatch(E.element,TEXT("Ring"));
        const FName Beam=E.hostile?TEXT("WarningBeam"):ElementBatch(E.element,TEXT("Beam"));
        const FVector At=World(E.position,E.hostile?8:3);
        const float Radius=FMath::Max(E.radius,.05f);
        if(IsAura(E)) continue;
        if(!E.hostile && E.form==Form::Beast) {
            Emit(TEXT("BeastCompanion"),At+FVector(0,0,55),FVector(.35),Heading(E.position-S.player.position));
            Emit(Ring,At,FVector(.35,.35,1));
            continue;
        }
        if(!E.hostile && E.kind==EffectKind::Burn) {
            // Visual-only coalescing: never mutate duration, radius, damage or core RNG.
            if(Radius>1.f) { Emit(Ring,At,FVector(Radius,Radius,1)); continue; }
            const FIntPoint Cell(FMath::FloorToInt(E.position.x/.8f),FMath::FloorToInt(E.position.y/.8f));
            if(!BurnCells.Contains(Cell)) {
                BurnCells.Add(Cell);
                const float Mark=FMath::Min(Radius,.65f);
                Emit(TEXT("BurnEmbers"),At,FVector(Mark,Mark,1),float(E.id%180));
            }
            continue;
        }
        if(E.kind==EffectKind::FireWall) {
            // Core shrinks a rectangular perimeter, not a radial blast or a beam to (0,0).
            const float Elapsed=Warning?0.f:FMath::Max(0.f,E.duration-E.life);
            const float X=48-Elapsed*5, Y=27-Elapsed*2.5f;
            for(float Sign:{-1.f,1.f}) {
                Emit(TEXT("WarningBeam"),FVector(Sign*(X-1)*100,0,8),FVector(.12,Y*2,.05));
                Emit(TEXT("WarningBeam"),FVector(0,Sign*(Y-1)*100,8),FVector(X*2,.12,.05));
                if(!Warning) {
                    Emit(TEXT("WarningBeam"),FVector(Sign*(X-.5f)*100,0,12),FVector(.3,Y*2,.08));
                    Emit(TEXT("WarningBeam"),FVector(0,Sign*(Y-.5f)*100,12),FVector(X*2,.3,.08));
                }
            }
        } else if(E.kind==EffectKind::Line) {
            if(!E.hostile && E.duration-E.life>.18f) continue;
            FVector End=World(E.end,At.Z); FVector D=End-At;
            if(Warning) {
                const FVector Side=FVector(-D.Y,D.X,0).GetSafeNormal()*Radius*100;
                for(float Sign:{-1.f,1.f}) Emit(Beam,(At+End)*.5f+Side*Sign,FVector(FMath::Max(float(D.Size()/100),.1f),.08,.035),D.Rotation().Yaw);
                Emit(Beam,End,FVector(.08,Radius*2,.035),D.Rotation().Yaw);
            } else Emit(Beam,(At+End)*.5f,FVector(FMath::Max(float(D.Size()/100),.1f),E.hostile?.08f:.035f,.025),D.Rotation().Yaw);
        } else if(E.kind==EffectKind::Slash && E.source==SpellId::FlameRing) {
            Emit(TEXT("FireShot"),At+FVector(0,0,60),FVector(.7),Heading(E.position-S.player.position)+90);
            // Orbiting blade silhouette is enough; no extra ring around each blade.
        } else if(E.kind==EffectKind::Slash) {
            FVector Direction=World(E.end)-World(E.position);
            if(Direction.IsNearlyZero()) Direction=World(S.player.facing);
            const float Facing=Direction.Rotation().Yaw;
            float Angle=120;
            for(const auto& Spell:S.progression.build.spells) if(Spell.id==SpellId::FlamePalm && Spell.rank>=3 && E.element==Element::Fire) Angle=150;
            const int32 Segments=12;
            for(int32 J=0;J<Segments;J++) {
                const float A=FMath::DegreesToRadians(Facing-Angle*.5f+Angle*J/Segments);
                const float B=FMath::DegreesToRadians(Facing-Angle*.5f+Angle*(J+1)/Segments);
                const FVector P=At+FVector(FMath::Cos(A),FMath::Sin(A),0)*Radius*100;
                const FVector Q=At+FVector(FMath::Cos(B),FMath::Sin(B),0)*Radius*100;
                const float Crest=FMath::Sin(PI*(J+.5f)/Segments);
                Emit(Beam,(P+Q)*.5f,FVector((Q-P).Size()/100,E.hostile?.09f:(.045f+.075f*Crest),E.hostile?.025f:.07f),(Q-P).Rotation().Yaw);
                // Raised, broken inner strokes sell a palm flame, without adding
                // another full-screen circle. Hostile bounds remain exact above.
                if(!E.hostile && E.element==Element::Fire && (J%2)==0) {
                    const FVector Inner=At+((P+Q)*.5f-At)*.84f+FVector(0,0,18+22*Crest);
                    Emit(TEXT("FireBeam"),Inner,FVector((Q-P).Size()/135,.035,.09),(Q-P).Rotation().Yaw-14);
                    Emit(TEXT("BurnEmbers"),Inner+FVector(0,0,24),FVector(.35,.35,.5),Facing+J*17);
                }
            }
        } else {
            // Exact hostile outer boundary, at a higher ground layer than friendly marks.
            if(E.hostile) Emit(Ring,At,FVector(Radius,Radius,1));
            else if(E.kind==EffectKind::Lightning) {
                if(Warning) Emit(TEXT("ThunderBoundary"),At,FVector(Radius,Radius,1));
                else if(E.duration-E.life<.18f) Emit(Beam,At,FVector(.045,.045,2.2));
            } else Emit(Ring,At,FVector(Radius,Radius,1));
        }
    }
}
