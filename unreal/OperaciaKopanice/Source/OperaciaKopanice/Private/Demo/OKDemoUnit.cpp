#include "Demo/OKDemoGameMode.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AStaticMeshActor* AOKDemoGameMode::BuildUnit(bool bGuard)
{
    auto* Unit=Mesh(FVector::ZeroVector,FVector::OneVector,FLinearColor::White);
    Unit->GetStaticMeshComponent()->SetStaticMesh(nullptr);
    const FLinearColor Cloth=bGuard ? FLinearColor(0.20f,0.25f,0.23f) : FLinearColor(0.30f,0.22f,0.15f);
    const FLinearColor Leather(0.08f,0.065f,0.055f);
    const FLinearColor Skin(0.58f,0.40f,0.28f);
    auto Part=[this,Unit](FVector P,FVector Size,FLinearColor Color,UStaticMesh* Asset,FRotator Rotation=FRotator::ZeroRotator)
    {
        auto* Component=NewObject<UStaticMeshComponent>(Unit);
        Unit->AddInstanceComponent(Component);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetupAttachment(Unit->GetRootComponent());
        Component->SetStaticMesh(Asset);
        Component->SetRelativeLocation(P);
        Component->SetRelativeScale3D(Size/100);
        Component->SetRelativeRotation(Rotation);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        auto* Material=UMaterialInstanceDynamic::Create(ShapeMaterial,Unit);
        Material->SetVectorParameterValue(TEXT("Color"),Color);
        Component->SetMaterial(0,Material);
        Component->RegisterComponent();
    };
    Part(FVector(0,0,1),FVector(76,76,3),bGuard ? FLinearColor(0.5f,0.12f,0.09f) : FLinearColor(0.07f,0.42f,0.31f),Cylinder);
    if (!bGuard)
    {
        if (auto* Imported=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Partisan/SM_OK_Partisan.SM_OK_Partisan")))
        {
            auto* Component=NewObject<UStaticMeshComponent>(Unit);
            Unit->AddInstanceComponent(Component);
            Component->SetMobility(EComponentMobility::Movable);
            Component->SetupAttachment(Unit->GetRootComponent());
            Component->SetStaticMesh(Imported);
            Component->SetRelativeLocation(FVector(0,0,3));
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetCanEverAffectNavigation(false);
            Component->ComponentTags.Add(TEXT("SuppliedPartisan"));
            Component->RegisterComponent();
            UE_LOG(LogTemp,Display,TEXT("OK_SUPPLIED_PARTISAN: imported mesh loaded"));
            return Unit;
        }
    }
    for (float Side : {-1.f,1.f})
    {
        Part(FVector(0,Side*12,46),FVector(19,18,72),Cloth,Sphere);
        Part(FVector(6,Side*12,13),FVector(32,19,24),Leather,Cube);
        Part(FVector(0,Side*27,116),FVector(18,18,54),Cloth,Sphere,FRotator(0,0,Side*12));
        Part(FVector(12,Side*29,91),FVector(26,16,17),Leather,Sphere);
    }
    Part(FVector(0,0,86),FVector(44,49,53),Cloth,Sphere);
    Part(FVector(0,0,120),FVector(38,48,59),Cloth,Sphere);
    Part(FVector(0,0,100),FVector(39,48,7),Leather,Cube);
    Part(FVector(20,0,101),FVector(4,9,8),FLinearColor(0.46f,0.42f,0.3f),Cube);
    Part(FVector(0,0,150),FVector(13,15,14),Skin,Cylinder);
    Part(FVector(1,0,165),FVector(25,23,31),Skin,Sphere);
    Part(FVector(13,0,166),FVector(8,7,8),Skin,Sphere);
    Part(FVector(-22,0,122),FVector(20,32,39),Leather,Cube);
    Part(FVector(-33,0,124),FVector(3,25,5),Cloth,Cube);
    if (bGuard)
    {
        Part(FVector(0,0,179),FVector(32,31,17),Cloth,Sphere);
        Part(FVector(1,0,175),FVector(35,34,5),Cloth,Cylinder);
        Part(FVector(21,26,116),FVector(7,8,68),FLinearColor(0.20f,0.12f,0.065f),Cube,FRotator(12,0,0));
        Part(FVector(31,26,163),FVector(4,4,47),Leather,Cylinder,FRotator(12,0,0));
    }
    else
    {
        Part(FVector(-2,0,178),FVector(29,29,12),Leather,Sphere);
        Part(FVector(12,0,176),FVector(19,26,4),Leather,Cube);
        Part(FVector(0,0,148),FVector(28,31,12),FLinearColor(0.35f,0.38f,0.34f),Sphere);
        Part(FVector(21,-17,90),FVector(10,14,19),Leather,Cube);
    }
    return Unit;
}
