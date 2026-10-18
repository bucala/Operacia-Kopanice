#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"

void AOKRTHUD::DrawTacticalMap(AOKRTGameMode* G,float W,float H)
{
    if (!bMapVisible || W<800 || H<650) return;
    const FVector2D P(W-236,148),Size(220,184);
    MapPanel=FBox2D(P+FVector2D(8,26),P+Size-FVector2D(8,8));
    Panels.Add(FBox2D(P,P+Size));
    DrawRect(FLinearColor(.025f,.035f,.035f,.93f),P.X,P.Y,Size.X,Size.Y);
    Label(TEXT("TAKTICKA MAPA"),P.X+9,P.Y+6,.85f,FLinearColor(.91f,.8f,.52f));
    DrawRect(FLinearColor(.18f,.24f,.23f,.9f),MapPanel.Min.X,MapPanel.Min.Y,MapPanel.GetSize().X,MapPanel.GetSize().Y);
    const auto Bounds=G->TacticalBounds();
    auto Project=[&](FVector2D World)
    {
        const FVector2D UV=(World-Bounds.Min)/Bounds.GetSize();
        return MapPanel.Min+FVector2D(FMath::Clamp(UV.X,0.,1.),FMath::Clamp(UV.Y,0.,1.))*MapPanel.GetSize();
    };
    if (!bMapBuilt)
    {
        bMapBuilt=true;
        for (TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        {
            const UStaticMesh* Mesh=It->GetStaticMeshComponent()->GetStaticMesh();
            if (!Mesh || !Mesh->GetName().Contains(TEXT("Cabin"))) continue;
            const auto B=It->GetStaticMeshComponent()->Bounds.GetBox();
            MapBuildings.Add(FBox2D(FVector2D(B.Min),FVector2D(B.Max)));
        }
    }
    for (const auto& B:MapBuildings)
    {
        const auto A=Project(B.Min),Z=Project(B.Max);
        DrawRect(FLinearColor(.53f,.57f,.52f),A.X,A.Y,FMath::Max(3.,Z.X-A.X),FMath::Max(3.,Z.Y-A.Y));
    }
    if (G->MissionId==0)
    {
        const auto A=Project(FVector2D(1080,Bounds.Min.Y)),B=Project(FVector2D(1080,Bounds.Max.Y));
        DrawLine(A.X,A.Y,B.X,B.Y,FLinearColor(.2f,.47f,.51f),170.f/Bounds.GetSize().X*MapPanel.GetSize().X);
        const auto C=Project(FVector2D(960,720)),D=Project(FVector2D(1200,720));
        DrawLine(C.X,C.Y,D.X,D.Y,G->bBridgeDestroyed ? FLinearColor(.85f,.25f,.19f) : FLinearColor(.79f,.81f,.7f),3);
    }
    const FVector Objectives[]={G->TNTLocation,G->DetonatorLocation,G->ExitLocation};
    for (int32 I=0;I<3;++I)
    {
        if ((I==0 && (G->bHasTNT || G->bObjectiveComplete)) || (I==1 && (G->bBridgeDestroyed || G->bObjectiveComplete || G->Mission().Goal==EOKMissionGoal::Documents))) continue;
        const auto A=Project(FVector2D(Objectives[I]));
        DrawRect(FLinearColor(.95f,.81f,.42f),A.X-3,A.Y-3,6,6);
    }
    for (AOKRTUnit* E:G->Enemies)
        if (E->IsAlive() && G->TeamSees(E))
        { const auto A=Project(FVector2D(E->Feet())); DrawRect(E->bUnconscious ? FLinearColor(.9f,.72f,.34f) : FLinearColor(.95f,.26f,.21f),A.X-2,A.Y-2,4,4); }
    for (int32 I=0;I<G->Party.Num();++I)
        if (G->Party[I]->IsAlive())
        {
            const auto A=Project(FVector2D(G->Party[I]->Feet()));
            DrawRect(I==G->ActiveMember ? FLinearColor(.99f,.91f,.64f) : FLinearColor(.4f,.9f,.62f),A.X-3,A.Y-3,6,6);
        }
    // The crosshair is the camera target, not an invented camera-frustum footprint.
    const auto C=Project(FVector2D(G->GetCameraTarget()));
    DrawLine(C.X-5,C.Y,C.X+5,C.Y,FLinearColor::White,1);
    DrawLine(C.X,C.Y-5,C.X,C.Y+5,FLinearColor::White,1);
}
void AOKRTHUD::DrawDrafts(AOKRTGameMode* G)
{
    for (int32 I=0;I<G->Party.Num();++I)
    {
        FOKRTOrder O; if (!G->GetDraft(I,O)) continue;
        const FLinearColor Color=I==0 ? FLinearColor(.96f,.79f,.36f,.95f) : FLinearColor(.45f,.93f,.7f,.95f);
        const FVector Target=O.Target.IsValid() ? O.Target->Feet() : O.Location;
        WorldRing(Target,46,Color);
        TArray<FVector> Points; FString Reason;
        if (!G->Party[I]->GetOrderPath(O,false,Points,Reason)) continue;
        for (int32 J=1;J<Points.Num();++J)
        {
            FVector2D A,B;
            if (PlayerOwner->ProjectWorldLocationToScreen(Points[J-1]+FVector(0,0,14),A) &&
                PlayerOwner->ProjectWorldLocationToScreen(Points[J]+FVector(0,0,14),B)) DrawLine(A.X,A.Y,B.X,B.Y,Color,2);
        }
    }
}
