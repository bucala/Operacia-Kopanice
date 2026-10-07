#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTVisionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

void AOKRTGameMode::SmokeForestTests()
{
    auto* Unit=Party[0].Get(); auto* Guard=Enemies[0].Get();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRTForestTest),false);
    for (AOKRTUnit* Member:Party) Params.AddIgnoredActor(Member);
    for (AOKRTUnit* Enemy:Enemies) Params.AddIgnoredActor(Enemy);
    int32 Trunks=0,Rocks=0;
    UCapsuleComponent* TestTrunk=nullptr;
    FVector Start,End,Side;
    for (TActorIterator<AActor> It(GetWorld());It;++It)
    {
        TArray<UCapsuleComponent*> Parts; It->GetComponents(Parts);
        for (auto* Part:Parts)
        {
            if (!Part->ComponentHasTag(TEXT("ForestTrunk"))) continue;
            ++Trunks;
            if (TestTrunk) continue;
            const FVector Base=Part->GetComponentLocation()-FVector(0,0,Part->GetScaledCapsuleHalfHeight());
            const FVector A=Base+FVector(-160,0,0),B=Base+FVector(160,0,0),C=Base+FVector(0,160,0);
            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByChannel(Hit,A+FVector(0,0,145),B+FVector(0,0,145),ECC_Visibility,Params) || Hit.Component.Get()!=Part) continue;
            if (GetWorld()->LineTraceTestByChannel(A+FVector(0,0,145),C+FVector(0,0,145),ECC_Visibility,Params)) continue;
            const auto Shape=FCollisionShape::MakeCapsule(30,90);
            if (GetWorld()->OverlapBlockingTestByChannel(A+FVector(0,0,92),FQuat::Identity,ECC_Pawn,Shape,Params) ||
                GetWorld()->OverlapBlockingTestByChannel(B+FVector(0,0,92),FQuat::Identity,ECC_Pawn,Shape,Params)) continue;
            auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),A,B,Unit);
            if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num()<3) continue;
            TestTrunk=Part; Start=A; End=B; Side=C;
        }
        TArray<UBoxComponent*> Boxes; It->GetComponents(Boxes);
        for (const auto* Part:Boxes) if (Part->ComponentHasTag(TEXT("ForestRock"))) ++Rocks;
    }
    Require(Trunks>100 && Rocks>15 && FoliageCover.Num()>30,TEXT("forest contains physical trunks rocks and shrub cover"));
    Require(TestTrunk!=nullptr,TEXT("NavMesh provides a detour around a real tree trunk"));
    if (SmokeStage<0) return;
    FHitResult Hit;
    Require(GetWorld()->SweepSingleByChannel(Hit,Start+FVector(0,0,92),End+FVector(0,0,92),FQuat::Identity,
        ECC_Pawn,FCollisionShape::MakeCapsule(30,90),Params) && Hit.Component.Get()==TestTrunk,
        TEXT("character capsule cannot pass through a tree trunk"));
    const FTransform UnitPose=Unit->GetActorTransform(),GuardPose=Guard->GetActorTransform();
    Guard->SetActorLocation(Start+FVector(0,0,Guard->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
    Guard->SetActorRotation(FRotator::ZeroRotator);
    Unit->SetActorLocation(End+FVector(0,0,Unit->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
    bool Primary=false;
    Require(!Guard->Vision->Sees(Unit,Primary),TEXT("tree trunk occludes authoritative guard vision"));
    Unit->SetActorLocation(Side+FVector(0,0,Unit->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
    Guard->SetActorRotation((Side-Start).Rotation());
    Require(Guard->Vision->Sees(Unit,Primary),TEXT("clear side of the same trunk remains visible"));
    Unit->SetActorTransform(UnitPose); Guard->SetActorTransform(GuardPose);

    FVector Shrub=Mission().Spawn+FVector(-180,-260,0); Shrub.Z=-8;
    Require(IsCover(Shrub,95) && !IsCover(Shrub+FVector(0,0,200),95),
        TEXT("authored shrub concealment has a finite vertical extent"));
    Require(!IsCover(Shrub+FVector(350,0,0),95),TEXT("shrub concealment ends outside the visible clump"));
    const EOKStance SavedStance=Unit->Stance;
    Unit->SetActorLocation(Shrub+FVector(0,0,Unit->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
    Unit->SetStance(EOKStance::Walk); Unit->Tick(0);
    Require(!Unit->bInCover && FMath::IsNearlyEqual(Unit->VisibilityFactor(),1.f),TEXT("standing in a shrub does not grant concealment"));
    Unit->SetStance(EOKStance::Crouch); Unit->Tick(0);
    Require(Unit->bInCover && FMath::IsNearlyEqual(Unit->VisibilityFactor(),.105f),TEXT("crouched shrub cover reduces detection without invisibility"));
    Unit->SetStance(EOKStance::Prone); Unit->Tick(0);
    Require(Unit->bInCover && FMath::IsNearlyEqual(Unit->VisibilityFactor(),.045f),TEXT("prone shrub cover retains a positive visibility factor"));
    Unit->SetStance(SavedStance); Unit->SetActorTransform(UnitPose); Unit->Tick(0);
    Require(!IsCover(FVector(600,1440,0)),TEXT("larger maps do not inherit the original mission's virtual cover"));
}
