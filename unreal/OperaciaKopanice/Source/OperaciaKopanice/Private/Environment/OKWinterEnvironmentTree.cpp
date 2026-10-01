#include "Environment/OKWinterEnvironmentActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Math/RotationMatrix.h"

void AOKWinterEnvironmentActor::BuildWinterTree(FVector Origin, int32 Seed, float Height)
{
    FRandomStream Random(1944 + Seed);
    const TCHAR* Path=Seed%2==0 ? TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowFir.SM_OK_SnowFir") :
        TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowFirSmall.SM_OK_SnowFirSmall");
    if (auto* Imported=LoadObject<UStaticMesh>(nullptr,Path))
    {
        auto* Tree=GetWorld()->SpawnActor<AStaticMeshActor>(Origin,FRotator(0,Random.FRandRange(0,360),0));
        auto* Component=Tree->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Imported);
        Component->SetCollisionProfileName(TEXT("NoCollision"));
        Component->SetCanEverAffectNavigation(false);
        Tree->SetActorScale3D(FVector(Height/FMath::Max(1.0,Imported->GetBoundingBox().GetSize().Z)));
        return;
    }
    const FLinearColor Bark(0.23f,0.25f,0.24f);
    const FLinearColor Snow(0.83f,0.90f,0.95f);
    auto Branch = [this, Origin](FVector Start, FVector End, float Diameter, FLinearColor Color)
    {
        const FVector Delta=End-Start;
        auto* Segment=Mesh(Origin+(Start+End)*0.5,FVector(Diameter/100,Diameter/100,Delta.Size()/100),Color,Cylinder);
        Segment->SetActorRotation(FRotationMatrix::MakeFromZ(Delta).Rotator());
    };
    // Deterministic bare deciduous crown: woody forks, fine twigs and snow on upper limbs.
    Branch(FVector(0,0,0),FVector(4,-3,150),17,Bark);
    Branch(FVector(4,-3,150),FVector(-4,2,285),10,Bark);
    for (int32 I=0; I<11; ++I)
    {
        const float Angle=I*2.39996f+Random.FRandRange(-0.12f,0.12f);
        const FVector Radial(FMath::Cos(Angle),FMath::Sin(Angle),0);
        const FVector Tangent(-Radial.Y,Radial.X,0);
        const float Z=110+I*13;
        const float Reach=Random.FRandRange(45,70);
        const FVector Root(0,0,Z);
        const FVector Fork=Root+Radial*Reach+FVector(0,0,35);
        Branch(Root,Fork,7-I*0.35f,Bark);
        Branch(Root+FVector(0,0,4),Fork+FVector(0,0,4),3.5f,Snow);
        for (float Side : {-1.f,1.f})
        {
            const FVector Tip=Fork+Radial*12+Tangent*Side*14+FVector(0,0,36);
            Branch(Fork,Tip,2.6f,Bark);
            Branch(Tip,Tip+Tangent*Side*8+FVector(0,0,17),1.2f,Bark);
        }
    }
    for (int32 I=0; I<4; ++I)
    {
        const float Angle=I*PI*0.5f;
        Branch(FVector(0,0,27),FVector(FMath::Cos(Angle)*28,FMath::Sin(Angle)*28,1),10,Bark);
    }
    Mesh(Origin+FVector(0,0,4),FVector(0.8,0.7,0.10),Snow,Sphere);
}
