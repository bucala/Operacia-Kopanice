#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

FVector FOKRTStoneFlight::Position(float Time) const
{
    const float Alpha=FMath::Clamp(Time/Duration,0.f,1.f);
    const float Arc=120.f+float(FVector::Dist2D(Origin,Destination))*.2f;
    return FMath::Lerp(Origin,Destination,Alpha)+FVector(0,0,4*Arc*Alpha*(1-Alpha));
}

bool AOKRTGameMode::StoneTrajectory(const AOKRTUnit* Unit,FVector From,FVector Destination,TArray<FVector>& Points) const
{
    Points.Reset();
    if (!Unit || From.ContainsNaN() || Destination.ContainsNaN() ||
        FVector::Dist2D(From,Destination)>800 || FMath::Abs(From.Z-Destination.Z)>200) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OKStoneArc),false,Unit);
    FHitResult Ground;
    if (!GetWorld()->LineTraceSingleByChannel(Ground,Destination+FVector(0,0,30),Destination-FVector(0,0,30),ECC_Visibility,Params) ||
        Ground.ImpactNormal.Z<.5 || Cast<AOKRTUnit>(Ground.GetActor())) return false;
    FOKRTStoneFlight Stone;
    Stone.Origin=From+FVector(0,0,Unit->Stance==EOKStance::Prone ? 45 : Unit->Stance==EOKStance::Crouch ? 90 : 125);
    Stone.Destination=Ground.ImpactPoint+FVector(0,0,6);
    Points.Add(Stone.Origin);
    for (int32 I=1;I<=24;++I)
    {
        const FVector Next=Stone.Position(float(I)/24);
        FHitResult Hit;
        if (GetWorld()->SweepSingleByChannel(Hit,Points.Last(),Next,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(2.5f),Params))
        { Points.Reset(); return false; }
        Points.Add(Next);
    }
    return true;
}

void AOKRTGameMode::CreateStoneVisual(FOKRTStoneFlight& Stone)
{
    Stone.Visual=GetWorld()->SpawnActor<AStaticMeshActor>();
    if (!Stone.Visual) return;
    auto* Mesh=Stone.Visual->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetCollisionProfileName(TEXT("NoCollision")); Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false);
    if (auto* Asset=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_ForestRock.SM_OK_ForestRock")))
    {
        Mesh->SetStaticMesh(Asset);
        const float Scale=6.f/FMath::Max(1.f,float(Asset->GetBounds().BoxExtent.GetMax()*2));
        Mesh->SetWorldScale3D(FVector(Scale));
    }
    Stone.Visual->SetActorLocation(Stone.Position(Stone.Elapsed));
}

bool AOKRTGameMode::Distract(AOKRTUnit* Unit,FVector Destination)
{
    TArray<FVector> Points;
    if (!Unit || !Unit->IsOperational() || Unit->bEnemy || Unit->CarriedBody.IsValid() || Unit->Distractions<=0 ||
        Stones.Num()>=16 || !StoneTrajectory(Unit,Unit->Feet(),Destination,Points))
    { Message=TEXT("Hod nema volnu drahu, podklad alebo je mimo dosahu."); return false; }
    FOKRTStoneFlight Stone; Stone.Origin=Points[0]; Stone.Destination=Points.Last();
    Stone.PartyMember=Party.IndexOfByKey(Unit);
    if (Stone.PartyMember==INDEX_NONE) return false;
    Stone.Duration=FMath::Clamp(float(FVector::Dist2D(Stone.Origin,Stone.Destination))/900.f,.45f,1.15f);
    CreateStoneVisual(Stone); Stones.Add(Stone);
    Message=TEXT("Kamen leti."); return true;
}

void AOKRTGameMode::UpdateStones(float Delta)
{
    if (bTacticalPause || bMenu || bWon || bLost || Delta<=0) return;
    for (int32 I=Stones.Num()-1;I>=0;--I)
    {
        auto& Stone=Stones[I];
        const float End=FMath::Min(Stone.Duration,Stone.Elapsed+Delta);
        bool Impact=false; FVector Location=Stone.Destination;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(OKStoneFlight),false,Party[Stone.PartyMember]);
        // Substeps keep fast throws from crossing thin/newly moved obstacles.
        while (Stone.Elapsed<End)
        {
            const float Next=FMath::Min(End,Stone.Elapsed+.025f); FHitResult Hit;
            if (GetWorld()->SweepSingleByChannel(Hit,Stone.Position(Stone.Elapsed),Stone.Position(Next),FQuat::Identity,
                ECC_Visibility,FCollisionShape::MakeSphere(2.5f),Params))
            { Impact=true; Location=Hit.ImpactPoint; break; }
            Stone.Elapsed=Next;
        }
        if (Impact || Stone.Elapsed>=Stone.Duration)
        {
            Noise(Party[Stone.PartyMember],Location,1,1500,TEXT("Distraction"));
            if (Stone.Visual) Stone.Visual->Destroy();
            Stones.RemoveAt(I); Message=TEXT("Dopad kamena.");
        }
        else if (Stone.Visual)
        {
            Stone.Visual->SetActorLocation(Stone.Position(Stone.Elapsed));
            Stone.Visual->AddActorWorldRotation(FRotator(Delta*360,Delta*240,0));
        }
    }
}

bool AOKRTGameMode::Whistle(AOKRTUnit* Unit)
{
    if (!Unit || Unit->bEnemy || !Unit->IsOperational() || Unit->Specialist!=EOKSpecialist::Partisan || Unit->CarriedBody.IsValid()) return false;
    Noise(Unit,Unit->Feet(),1,700,TEXT("Whistle"));
    Message=TEXT("Partizan zapiskal."); return true;
}

bool AOKRTGameMode::NoiseAudible(const AOKRTUnit* Listener,const AOKRTUnit* Source,FVector Location,float Range) const
{
    if (!Listener || !Source || !Listener->IsOperational() || !FMath::IsFinite(Range) || Range<=0) return false;
    const float Distance=FVector::Dist(Listener->Feet(),Location);
    if (Distance>Range) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OKAcoustics),false,Listener);
    for (const auto& U:Party) Params.AddIgnoredActor(U);
    for (const auto& U:Enemies) Params.AddIgnoredActor(U);
    FHitResult Hit;
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Listener->Feet()+FVector(0,0,145),Location+FVector(0,0,35),ECC_Visibility,Params);
    return Distance<=Range*(Blocked ? .3f : 1.f);
}
