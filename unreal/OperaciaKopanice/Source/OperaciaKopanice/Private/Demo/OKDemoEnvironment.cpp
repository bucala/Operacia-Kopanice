#include "Demo/OKDemoGameMode.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

void AOKDemoGameMode::BuildEnvironment()
{
    auto* Ground=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/WinterGround/SM_OK_WinterGround.SM_OK_WinterGround"));
    auto* Cobbles=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/WinterGround/SM_OK_WinterCobbles.SM_OK_WinterCobbles"));
    OverlayMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_TacticalOverlay.M_TacticalOverlay"));
    bContinuousTerrain=Ground && Cobbles && OverlayMaterial;
    if (!bContinuousTerrain)
    {
        Mesh(FVector(900,720,-75),FVector(23,19,.8),FLinearColor(.48f,.56f,.60f));
        UE_LOG(LogTemp,Warning,TEXT("OK_ENVIRONMENT: winter ground assets missing; using blockout"));
        return;
    }
    auto Place=[this](UStaticMesh* Asset,FVector Location,FRotator Rotation,float Scale)
    {
        auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(Location,Rotation);
        auto* Component=Actor->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Asset);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Actor->SetActorScale3D(FVector(Scale));
        return Actor;
    };
    Place(Ground,FVector::ZeroVector,FRotator::ZeroRotator,1);
    Place(Cobbles,FVector::ZeroVector,FRotator::ZeroRotator,1);
    if (auto* Cabin=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Cabin/SM_OK_Cabin.SM_OK_Cabin")))
    {
        // Background buildings do not introduce new tactical cover or block walkable cells.
        const FVector Size=Cabin->GetBoundingBox().GetSize();
        const float Scale=720.f/FMath::Max(Size.X,Size.Y);
        for (const FVector P : {FVector(230,-660,-47),FVector(1740,-660,-47),
            FVector(-680,360,-47),FVector(2540,320,-47)})
        {
            auto* Building=Place(Cabin,P,FRotator::ZeroRotator,Scale);
            Building->Tags.Add(TEXT("Cabin4x4"));
        }
    }
    const FVector Trees[]={
        {-370,-360,8},{-730,-50,10},{-550,700,8},{-710,1000,16},{-520,1590,10},
        {30,-760,24},{570,-820,24},{840,-620,16},{1300,-700,18},{2090,-620,24},
        {2260,-130,12},{2520,730,18},{2250,1150,14},{2550,1510,28},
        {-960,270,20},{-990,1250,22},{2740,180,26},{2920,1130,30}};
    auto ClearOfBuildings=[](FVector P)
    {
        for (const FVector B : {FVector(230,-660,0),FVector(1740,-660,0),
            FVector(-680,360,0),FVector(2540,320,0)})
            if (FMath::Abs(P.X-B.X)<500 && FMath::Abs(P.Y-B.Y)<500) return false;
        return true;
    };
    for (int32 I=0; I<UE_ARRAY_COUNT(Trees); ++I)
        if (ClearOfBuildings(Trees[I])) BuildWinterTree(Trees[I],100+I,510.f+(I%4)*35.f);
    // A second, staggered background row adds depth while leaving the board unobstructed.
    for (int32 I=0; I<10; ++I)
        BuildWinterTree(FVector(-1050+I*370,-1400-(I%3)*100,24),150+I,580.f+(I%3)*40.f);
    if (auto* Shrub=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowShrub.SM_OK_SnowShrub")))
    {
        for (int32 I=0; I<UE_ARRAY_COUNT(Trees); ++I)
            if (ClearOfBuildings(Trees[I]))
                Place(Shrub,Trees[I]+FVector(90,-80,-5),FRotator(0,I*47,0),.65f+(I%3)*.13f);
    }
    if (auto* Rock=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_ForestRock.SM_OK_ForestRock")))
    {
        const FVector Rocks[]={{-320,230,-4},{-430,1180,0},{2100,80,4},{2120,1430,2},
            {890,-320,0},{1250,1740,0},{-960,570,12}};
        for (int32 I=0; I<UE_ARRAY_COUNT(Rocks); ++I)
            Place(Rock,Rocks[I],FRotator(0,I*73,0),.65f+(I%3)*.2f);
    }
    // Split-rail fencing frames the settlement without enclosing the playable board.
    for (int32 I=0; I<7; ++I)
    {
        const FVector P(-300+I*115,-225,0);
        Mesh(P+FVector(0,0,45),FVector(.10,.10,.95),FLinearColor(.18f,.16f,.13f));
        if (I<6)
            for (float Z : {35.f,65.f})
                Mesh(P+FVector(57,0,Z),FVector(1.16,.07,.09),FLinearColor(.23f,.21f,.18f));
    }
    UE_LOG(LogTemp,Display,TEXT("OK_ENVIRONMENT: continuous snow, cobbles and settlement loaded"));
}
