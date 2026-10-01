#include "Environment/OKWinterEnvironmentActor.h"
#include "Demo/OKObjectLayout.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

bool AOKWinterEnvironmentActor::BuildEnvironment()
{
    auto* Ground=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/WinterGround/SM_OK_WinterGround.SM_OK_WinterGround"));
    auto* Cobbles=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/WinterGround/SM_OK_WinterCobbles.SM_OK_WinterCobbles"));
    const bool bContinuousTerrain=Ground && Cobbles;
    if (!bContinuousTerrain)
    {
        Mesh(FVector(900,720,-75),FVector(23,19,.8),FLinearColor(.48f,.56f,.60f));
        UE_LOG(LogTemp,Warning,TEXT("OK_ENVIRONMENT: winter ground assets missing; using blockout"));
        return false;
    }
    auto Place=[this](UStaticMesh* Asset,FVector Location,FRotator Rotation,float Scale)
    {
        auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(Location,Rotation);
        auto* Component=Actor->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Asset);
        Component->SetCollisionProfileName(TEXT("NoCollision"));
        Component->SetCanEverAffectNavigation(false);
        Actor->SetActorScale3D(FVector(Scale));
        return Actor;
    };
    Place(Ground,FVector::ZeroVector,FRotator::ZeroRotator,1);
    Place(Cobbles,FVector::ZeroVector,FRotator::ZeroRotator,1);
    auto* BackdropPlane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* Snow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_Snow.M_Snow"));
    if (BackdropPlane && Snow)
    {
        // Distant snow fills tall views; it stays below the carved river and has no gameplay role.
        auto* Backdrop=Place(BackdropPlane,FVector(900,720,-200),FRotator::ZeroRotator,3000.f);
        Backdrop->GetStaticMeshComponent()->SetMaterial(0,Snow);
        Backdrop->GetStaticMeshComponent()->SetCastShadow(false);
        Backdrop->Tags.Add(TEXT("WinterBackdrop"));
    }
    if (auto* Paths=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/WinterGround/SM_OK_WoodenPaths.SM_OK_WoodenPaths")))
        Place(Paths,FVector::ZeroVector,FRotator::ZeroRotator,1)->Tags.Add(TEXT("ReferenceWoodenPaths"));
    if (auto* Cabin=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Cabin/SM_OK_Cabin.SM_OK_Cabin")))
    {
        // Background buildings do not introduce new tactical cover or block walkable cells.
        const FVector Size=Cabin->GetBoundingBox().GetSize();
        const float Scale=OKLayout::UniformScale(OKLayout::EObject::Cabin,Size.X,Size.Y);
        auto* Alternate=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/CabinAlt/SM_OK_CabinAlt.SM_OK_CabinAlt"));
        int32 Index=0;
        for (const FVector P : {FVector(230,-660,-47),FVector(1740,-660,-47),
            FVector(-680,360,-47),FVector(2540,320,-47)})
        {
            const bool bAlternate=Alternate && (Index==1 || Index==2);
            auto* Asset=bAlternate ? Alternate : Cabin;
            const FVector Bounds=Asset->GetBoundingBox().GetSize();
            auto* Building=Place(Asset,P,FRotator(0,Index==2 ? 90 : Index==3 ? 180 : 0,0),
                bAlternate ? OKLayout::UniformScale(OKLayout::EObject::Cabin,Bounds.X,Bounds.Y) : Scale);
            Building->Tags.Add(TEXT("Cabin4x4"));
            if (bAlternate) Building->Tags.Add(TEXT("SuppliedCabinAlt"));
            ++Index;
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
    for (int32 I=0;I<24;++I)
    {
        const float X=I<12 ? -1400.f-(I%3)*105.f : 3020.f+(I%3)*105.f;
        const float Y=-850.f+(I%12)*235.f+FMath::Sin(I*2.4f)*90.f;
        BuildWinterTree(FVector(X,Y,24),220+I,530.f+(I%5)*37.f);
    }
    if (auto* Van=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/CommandVan/SM_OK_CommandVan.SM_OK_CommandVan")))
    {
        const FVector Size=Van->GetBoundingBox().GetSize();
        auto* Vehicle=Place(Van,FVector(2550,1350,8),FRotator(0,0,0),
            OKLayout::UniformScale(OKLayout::EObject::Truck,Size.X,Size.Y));
        Vehicle->Tags.Add(TEXT("SuppliedCommandVan"));
    }
    if (auto* Crate=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/WinterGround/SM_OK_SupplyCrate.SM_OK_SupplyCrate")))
    {
        const FVector Supplies[]={{-250,70,16},{-250,135,16},{490,-200,16},{550,-200,16},
            {2180,640,16},{2180,710,16},{2130,710,57},{2320,1180,16},{2390,1195,16},
            {-400,1000,16},{-420,1080,16},{550,-1000,36}};
        for (int32 I=0;I<UE_ARRAY_COUNT(Supplies);++I)
            Place(Crate,Supplies[I],FRotator(0,(I%3)*12,0),1)->Tags.Add(TEXT("ReferenceSupplies"));
    }
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
        // Keep small shore rocks inside the blocked river column and clear all crossings.
        for (int32 Row : {0,2,6,8})
            for (int32 Side : {-1,1})
                Place(Rock,FVector(1080+Side*63,Row*180+25,-28),FRotator(0,Row*43,0),.18f);
        for (int32 I=0;I<20;++I)
        {
            const float Y=I<10 ? -240.f-I*180.f : 1700.f+(I-10)*180.f;
            const float Exterior=FMath::Clamp(FMath::Max(-90.f-Y,Y-1530.f)/600.f,0.f,1.f);
            const float X=1080+Exterior*230*FMath::Sin(Y*.0025f);
            Place(Rock,FVector(X+(I%2 ? 92 : -92),Y,-16),FRotator(0,I*73,0),.28f+(I%3)*.07f);
        }
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
    return true;
}


#include "Materials/MaterialInstanceDynamic.h"

AOKWinterEnvironmentActor::AOKWinterEnvironmentActor()
{
    Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    ShapeMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}
AStaticMeshActor* AOKWinterEnvironmentActor::Mesh(FVector Location, FVector Scale, FLinearColor Color, UStaticMesh* Asset)
{
    auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(Location,FRotator::ZeroRotator);
    auto* Component=Actor->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Asset ? Asset : Cube.Get());
    Component->SetCollisionProfileName(TEXT("NoCollision"));
    Component->SetCanEverAffectNavigation(false);
    Component->SetMaterial(0,ShapeMaterial);
    Actor->SetActorScale3D(Scale);
    if (auto* Material=Component->CreateDynamicMaterialInstance(0))
        Material->SetVectorParameterValue(TEXT("Color"),Color);
    return Actor;
}
