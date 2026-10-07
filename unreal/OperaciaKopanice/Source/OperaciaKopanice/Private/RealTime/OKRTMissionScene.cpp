#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

AStaticMeshActor* AOKRTGameMode::MissionProp(const TCHAR* Path,FVector Center,FVector2D Footprint,float Yaw,bool Blocking,FName Tag)
{
    auto* Asset=LoadObject<UStaticMesh>(nullptr,Path);
    if (!Asset) return nullptr;
    const FBox Box=Asset->GetBoundingBox();
    const FVector Size=Box.GetSize();
    // Imported props use different forward axes. Fit vehicles along their long
    // dimension while retaining their proportions and the requested world yaw.
    const bool Swap=Footprint.X>Footprint.Y && Size.Y>Size.X;
    const float Scale=Swap ? FMath::Min(Footprint.X/Size.Y,Footprint.Y/Size.X) : FMath::Min(Footprint.X/Size.X,Footprint.Y/Size.Y);
    const FRotator Rotation(0,Yaw+(Swap ? 90 : 0),0);
    const FVector Pivot(Box.GetCenter().X,Box.GetCenter().Y,Box.Min.Z);
    auto* Actor=Place(Path,Center-Rotation.RotateVector(Pivot*Scale),Scale);
    Actor->SetActorRotation(Rotation); Actor->Tags.Add(Tag);
    if (Blocking)
    {
        const FVector Extent(Size.X*Scale*.46f,Size.Y*Scale*.46f,Size.Z*Scale*.5f);
        auto* BoxCollider=Collider(Center+FVector(0,0,Extent.Z),Extent,Tag);
        BoxCollider->SetWorldRotation(Rotation);
    }
    return Actor;
}
void AOKRTGameMode::SpawnMissionUnit(FVector Location,bool Enemy,float Yaw,const TArray<FOKRTPatrolStop>& Route)
{
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Unit=GetWorld()->SpawnActor<AOKRTUnit>(Location,FRotator(0,Yaw,0),Params);
    Unit->Initialize(Enemy,Enemy || !Party.IsEmpty());
    if (Enemy)
    {
        Enemies.Add(Unit);
        auto* AI=CastChecked<AOKRTGuardController>(Unit->GetController());
        AI->PatrolRoute=Route; AI->bBrainEnabled=false;
    }
    else Party.Add(Unit);
}
void AOKRTGameMode::BuildAdditionalMission()
{
    const auto& Definition=Mission();
    const float Size=Definition.MapSize;
    auto* Ground=Place(TEXT("/Engine/BasicShapes/Plane.Plane"),FVector(Size/2,Size/2,-8),Size/100);
    Ground->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_ForestSnow.M_ForestSnow")));
    Ground->GetStaticMeshComponent()->SetCastShadow(false);
    Ground->Tags.Add(TEXT("MissionTerrain"));
    auto* Backdrop=Place(TEXT("/Engine/BasicShapes/Plane.Plane"),FVector(Size/2,Size/2,-10),3000);
    Backdrop->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_ForestSnow.M_ForestSnow")));
    Backdrop->GetStaticMeshComponent()->SetCastShadow(false);
    Collider(FVector(Size/2,Size/2,-18),FVector(Size/2,Size/2,10),TEXT("MissionWalkable"));
    const TCHAR* Cabin=TEXT("/Game/Kopanice/Supplied/Cabin/SM_OK_Cabin.SM_OK_Cabin");
    const TCHAR* CabinAlt=TEXT("/Game/Kopanice/Supplied/CabinAlt/SM_OK_CabinAlt.SM_OK_CabinAlt");
    const TCHAR* Van=TEXT("/Game/Kopanice/Supplied/CommandVan/SM_OK_CommandVan.SM_OK_CommandVan");
    const TCHAR* Car=TEXT("/Game/Kopanice/Supplied/Car/SM_OK_Car.SM_OK_Car");
    const TCHAR* Crate=TEXT("/Game/Kopanice/WinterGround/SM_OK_SupplyCrate.SM_OK_SupplyCrate");
    const FVector Huts2[]={{2450,1700,0},{4500,3200,0},{5700,4700,0},{3100,5700,0}};
    const FVector Huts3[]={{2200,5500,0},{3500,4400,0},{5500,1700,0},{5800,5400,0},{2800,900,0}};
    const TArray<FVector> Huts=MissionId==1 ? TArray<FVector>(Huts2,UE_ARRAY_COUNT(Huts2)) : TArray<FVector>(Huts3,UE_ARRAY_COUNT(Huts3));
    const FVector Stops2[]={{3500,2400,0},{2600,4400,0},{5800,5400,0}};
    const FVector Stops3[]={{4300,3900,0},{5700,2500,0},{3200,1400,0}};
    for (int32 I=0;I<Huts.Num();++I)
        MissionProp(I%2 ? Cabin : CabinAlt,Huts[I],FVector2D(720,720),(I%4)*90,true,TEXT("MissionCabin"));
    if (MissionId==1)
    {
        MissionProp(Car,FVector(4600,3900,0),FVector2D(540,360),90,true,TEXT("MissionVehicle"));
        MissionProp(Van,FVector(5600,3600,0),FVector2D(900,360),0,true,TEXT("MissionVehicle"));
    }
    else MissionTargetActor=MissionProp(Van,FVector(4800,2600,0),FVector2D(900,360),90,true,TEXT("MissionCommandPost"));
    // These imported courtyard/path meshes remain visual; proxies own all navigation.
    MissionProp(TEXT("/Game/Kopanice/WinterGround/SM_OK_WinterCobbles.SM_OK_WinterCobbles"),
        MissionId==1 ? FVector(3600,3300,-5) : FVector(3900,3500,-5),FVector2D(3500,4000),0,false,TEXT("MissionRoad"));
    MissionProp(TEXT("/Game/Kopanice/WinterGround/SM_OK_WoodenPaths.SM_OK_WoodenPaths"),
        FVector(2900,3800,0),FVector2D(2700,3300),90,false,TEXT("MissionPaths"));
    TNTMarker=MissionProp(Crate,TNTLocation,FVector2D(90,90),0,false,TEXT("MissionSupplies"));
    for (const FVector P:{HideLocation+FVector(90,0,0),FVector(1900,3700,0),FVector(3500,2800,0),FVector(3800,5100,0)})
    {
        MissionProp(Crate,P,FVector2D(120,120),0,true,TEXT("StealthCover"));
        CoverLocations.Add(P);
        MissionProp(Crate,P+FVector(0,145,0),FVector2D(100,100),12,true,TEXT("StealthCover"));
    }
    // Render meshes stay instanced. Simple shared-owner collision components
    // describe trunks/rocks, not opaque boxes around the entire tree canopy.
    enum class EForestPart { Tree, Shrub, Rock };
    auto AddForest=[&](const TCHAR* Path,float Height,int32 Count,int32 Seed,EForestPart Kind)
    {
        auto* Asset=LoadObject<UStaticMesh>(nullptr,Path);
        if (!Asset) return;
        // GameMode actors are hidden in game. Forest render components need an
        // independent visible owner, not the coordinator's spline root.
        auto* ForestActor=GetWorld()->SpawnActor<AActor>();
        auto* Instances=NewObject<UHierarchicalInstancedStaticMeshComponent>(ForestActor);
        ForestActor->AddInstanceComponent(Instances); ForestActor->SetRootComponent(Instances);
        Instances->SetStaticMesh(Asset); Instances->SetCollisionProfileName(TEXT("NoCollision"));
        Instances->SetCanEverAffectNavigation(false); Instances->RegisterComponent();
        const FBox Box=Asset->GetBoundingBox();
        FRandomStream Random(Seed);
        for (int32 I=0;I<Count+(Kind==EForestPart::Shrub ? 2 : 0);++I)
        {
            const bool Authored=I>=Count;
            FVector P=Authored ? (I==Count ? Definition.Spawn+FVector(-180,-260,0) : HideLocation+FVector(-220,-120,0)) :
                FVector(Random.FRandRange(250,Size-250),Random.FRandRange(250,Size-250),0);
            P.Z=-8;
            if (!Authored && ((P.X>700 && P.X<1900 && P.Y<7000) ||
                FVector::Dist2D(P,TNTLocation)<380 || FVector::Dist2D(P,DetonatorLocation)<500 ||
                FVector::Dist2D(P,ExitLocation)<400 || FVector::Dist2D(P,Definition.Spawn)<500)) continue;
            bool Clear=true;
            for (const FVector Hut:Huts) if (FVector::Dist2D(P,Hut)<650) Clear=false;
            for (int32 Stop=0;Stop<3;++Stop)
                if (FVector::Dist2D(P,MissionId==1 ? Stops2[Stop] : Stops3[Stop])<350) Clear=false;
            if (MissionId==2 && FMath::Abs(P.X-4300)<160 && P.Y>2600 && P.Y<4100) Clear=false;
            if (!Clear) continue;
            const float Clearance=Kind==EForestPart::Rock ? 100.f : 55.f;
            if (GetWorld()->OverlapBlockingTestByChannel(P+FVector(0,0,110),FQuat::Identity,ECC_Pawn,
                FCollisionShape::MakeCapsule(Clearance,100))) continue;
            const float Scale=Height/Box.GetSize().Z*(Authored ? 1.35f : Random.FRandRange(.8f,1.2f));
            const FRotator Rotation(0,Random.FRandRange(0,360),0);
            const FVector Pivot(Box.GetCenter().X,Box.GetCenter().Y,Box.Min.Z);
            Instances->AddInstance(FTransform(Rotation,P-Rotation.RotateVector(Pivot*Scale),FVector(Scale)),true);
            UShapeComponent* Collision=nullptr;
            if (Kind==EForestPart::Tree)
            {
                auto* Trunk=NewObject<UCapsuleComponent>(ForestActor);
                const float Radius=FMath::Clamp(float(Box.GetSize().Z*Scale)*.035f,16.f,28.f);
                const float HalfHeight=FMath::Min(float(Box.GetSize().Z*Scale)*.3f,200.f);
                Trunk->SetCapsuleSize(Radius,HalfHeight);
                Trunk->SetRelativeLocation(P+FVector(0,0,HalfHeight));
                Trunk->ComponentTags.Add(TEXT("ForestTrunk")); Collision=Trunk;
            }
            else if (Kind==EForestPart::Rock)
            {
                auto* Rock=NewObject<UBoxComponent>(ForestActor);
                const FVector Extent=Box.GetSize()*Scale*FVector(.4f,.4f,.5f);
                Rock->SetBoxExtent(Extent); Rock->SetRelativeLocation(P+FVector(0,0,Extent.Z));
                Rock->SetRelativeRotation(Rotation);
                Rock->ComponentTags.Add(TEXT("ForestRock")); Collision=Rock;
            }
            else FoliageCover.Add({P,FMath::Clamp(float(FMath::Min(Box.GetSize().X,Box.GetSize().Y)*Scale)*.28f,45.f,135.f),float(Box.GetSize().Z*Scale)});
            if (Collision)
            {
                ForestActor->AddInstanceComponent(Collision); Collision->SetupAttachment(Instances);
                Collision->SetCollisionProfileName(TEXT("BlockAll"));
                Collision->SetCanEverAffectNavigation(true); Collision->SetGenerateOverlapEvents(false);
                Collision->CanCharacterStepUpOn=ECB_No; Collision->RegisterComponent();
            }
        }
        Instances->ComponentTags.Add(TEXT("MissionForest"));
    };
    AddForest(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowFir.SM_OK_SnowFir"),650,280,41+MissionId,EForestPart::Tree);
    AddForest(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowFirSmall.SM_OK_SnowFirSmall"),420,160,73+MissionId,EForestPart::Tree);
    AddForest(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_SnowShrub.SM_OK_SnowShrub"),100,150,91+MissionId,EForestPart::Shrub);
    AddForest(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_ForestRock.SM_OK_ForestRock"),150,80,117+MissionId,EForestPart::Rock);
    // A few tactical rocks have real collision and provide crouched concealment.
    for (const FVector P:{FVector(2100,4900,0),FVector(2100,3500,0),FVector(3800,2000,0)})
    {
        MissionProp(TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_ForestRock.SM_OK_ForestRock"),P,FVector2D(200,170),35,true,TEXT("StealthCover"));
        CoverLocations.Add(P);
    }
    SpawnMissionUnit(Definition.Spawn,false,0,{});
    SpawnMissionUnit(Definition.Spawn+FVector(180,0,0),false,0,{});
    for (int32 I=0;I<3;++I)
    {
        const FVector Stop=MissionId==1 ? Stops2[I] : Stops3[I];
        SpawnMissionUnit(Stop+FVector(0,0,100),true,0,{
            {Stop,0,12},{Stop+FVector(180,0,0),90,2},{Stop+FVector(180,0,0),-90,1}});
    }
    Select(0); CameraTarget=Definition.Spawn+FVector(700,-700,0); CameraTarget.Z=0;
    CameraDistance=5200;
    UE_LOG(LogTemp,Display,TEXT("OK_RT_MISSION_SCENE: id=%d size=%.0f huts=%d guards=%d"),MissionId,Size,Huts.Num(),Enemies.Num());
}
