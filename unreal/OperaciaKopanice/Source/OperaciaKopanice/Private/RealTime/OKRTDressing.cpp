#include "RealTime/OKRTGameMode.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

void AOKRTGameMode::DressScene()
{
    TArray<FBox> Cabins;
    for (TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
    {
        const UStaticMesh* Mesh=It->GetStaticMeshComponent()->GetStaticMesh();
        if (Mesh && Mesh->GetName().Contains(TEXT("Cabin"))) Cabins.Add(It->GetStaticMeshComponent()->Bounds.GetBox());
    }
    auto Reserved=[this](FVector P)
    {
        const auto Bounds=TacticalBounds();
        if (!Bounds.IsInside(FVector2D(P))) return true;
        if (MissionId==0 && P.X>920 && P.X<1240) return true;
        for (const FVector Goal:{TNTLocation,DetonatorLocation,HideLocation,ExitLocation,Mission().Spawn})
            if (FVector::Dist2D(P,Goal)<200) return true;
        for (const auto& Guard:Mission().Guards)
        {
            FVector Previous=Guard.Spawn;
            for (const auto& Stop:Guard.Route)
            {
                if (FMath::PointDistToSegment(P,Previous,Stop.Location)<170) return true;
                Previous=Stop.Location;
            }
        }
        return false;
    };
    int32 Instances=0;
    auto Cluster=[&](const TCHAR* Path,float Width,int32 PerCabin,int32 Seed)
    {
        auto* Asset=LoadObject<UStaticMesh>(nullptr,Path); if (!Asset) return;
        auto* Owner=GetWorld()->SpawnActor<AActor>();
        auto* Mesh=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
        Owner->SetRootComponent(Mesh); Owner->AddInstanceComponent(Mesh);
        Mesh->SetStaticMesh(Asset); Mesh->SetCollisionProfileName(TEXT("NoCollision"));
        Mesh->SetCanEverAffectNavigation(false); Mesh->RegisterComponent();
        Owner->Tags.Add(TEXT("AtmosphericDressing"));
        FRandomStream Random(Seed+MissionId);
        const FBox Local=Asset->GetBoundingBox(); const FVector Extent=Local.GetSize();
        for (const FBox& Cabin:Cabins) for (int32 I=0;I<PerCabin;++I)
        {
            const FVector2D Size(Cabin.GetSize());
            const FVector2D Offset=I%2 ? FVector2D(-Size.X*.5-50-Random.FRandRange(0,65),Random.FRandRange(-Size.Y*.4,Size.Y*.4)) :
                FVector2D(Random.FRandRange(-Size.X*.4,Size.X*.4),-Size.Y*.5-50-Random.FRandRange(0,65));
            const FVector P=Cabin.GetCenter()+FVector(Offset.X,Offset.Y,-Cabin.GetCenter().Z);
            if (Reserved(P)) continue;
            const float Scale=Width/FMath::Max(Extent.X,Extent.Y)*Random.FRandRange(.8f,1.15f);
            const FQuat Rotation(FVector::UpVector,Random.FRandRange(0,2*PI));
            const FVector Pivot(Local.GetCenter().X,Local.GetCenter().Y,Local.Min.Z);
            Mesh->AddInstance(FTransform(Rotation,P-Rotation.RotateVector(Pivot*Scale),FVector(Scale)));
            ++Instances;
        }
    };
    // Small perimeter dressing stays non-blocking. Gameplay cover remains owned
    // by the existing tested collision proxies and foliage-cover volumes.
    Cluster(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowShrub.SM_OK_SnowShrub"),90,10,337);
    Cluster(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_ForestRock.SM_OK_ForestRock"),100,4,619);
    Cluster(TEXT("/Game/Kopanice/WinterGround/SM_OK_SupplyCrate.SM_OK_SupplyCrate"),65,3,853);
    UE_LOG(LogTemp,Display,TEXT("OK_RT_DRESSING: cabins=%d instances=%d collision=visual-only"),Cabins.Num(),Instances);
}
