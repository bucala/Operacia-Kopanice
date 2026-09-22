#include "Navigation/OKViaductActor.h"
#include "Navigation/OKDynamicNavObstacleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "NavigationSystem.h"
#include "NavAreas/NavArea_Null.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

AOKViaductActor::AOKViaductActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    IntactSpan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactSpan"));
    IntactSpan->SetupAttachment(RootComponent);
    Rubble = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rubble"));
    Rubble->SetupAttachment(RootComponent);
    GapBlocker = CreateDefaultSubobject<UOKDynamicNavObstacleComponent>(TEXT("GapBlocker"));
    GapBlocker->SetupAttachment(RootComponent);
    GapBlocker->SetBoxExtent(FVector(500, 200, 150));
    GapBlocker->SetAreaClassOverride(UNavArea_Null::StaticClass());
}

void AOKViaductActor::BeginPlay() { Super::BeginPlay(); ApplyState(); }
void AOKViaductActor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); ApplyState(); }
void AOKViaductActor::DestroySpan() { bDestroyed = true; ApplyState(); }
void AOKViaductActor::RepairSpan() { bDestroyed = false; ApplyState(); }

void AOKViaductActor::ApplyState()
{
    for (UStaticMeshComponent* Part : SpanDetails) Part->SetVisibility(!bDestroyed);
    for (UStaticMeshComponent* Part : DebrisDetails) Part->SetVisibility(bDestroyed);
    IntactSpan->SetVisibility(!bDestroyed);
    IntactSpan->SetCollisionEnabled(bDestroyed ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    IntactSpan->SetCanEverAffectNavigation(!bDestroyed);
    Rubble->SetVisibility(bDestroyed);
    Rubble->SetCollisionEnabled(bDestroyed ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Rubble->SetCanEverAffectNavigation(bDestroyed);
    GapBlocker->bStartsBlocked = bDestroyed;
    GapBlocker->SetObstacleActive(bDestroyed);
    UNavigationSystemV1::UpdateComponentInNavOctree(*IntactSpan);
    UNavigationSystemV1::UpdateComponentInNavOctree(*Rubble);
}

void AOKViaductActor::BuildDemoModel()
{
    for (UStaticMeshComponent* Part : DemoParts)
    {
        RemoveInstanceComponent(Part);
        Part->DestroyComponent();
    }
    DemoParts.Reset(); SpanDetails.Reset(); DebrisDetails.Reset();
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Material=[this,Base](FLinearColor Color)
    {
        auto* M=UMaterialInstanceDynamic::Create(Base,this);
        M->SetVectorParameterValue(TEXT("Color"),Color);
        return M;
    };
    auto* Concrete=Material(FLinearColor(0.32f,0.35f,0.36f));
    auto* Steel=Material(FLinearColor(0.13f,0.17f,0.18f));
    auto* Snow=Material(FLinearColor(0.78f,0.86f,0.91f));
    IntactSpan->SetStaticMesh(Cube);
    IntactSpan->SetRelativeScale3D(FVector(1.8,1.5,0.22));
    IntactSpan->SetMaterial(0,Concrete);
    // This compact model occupies one demo crossing, not the full mission-scale viaduct.
    GapBlocker->SetBoxExtent(FVector(90,75,100));
    auto Part=[this,Cube](FVector P,FVector Size,UMaterialInterface* M,int32 Group,FRotator R=FRotator::ZeroRotator)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(C);
        C->SetMobility(EComponentMobility::Movable);
        C->SetupAttachment(RootComponent);
        C->SetStaticMesh(Cube);
        C->SetMaterial(0,M);
        C->SetRelativeLocation(P);
        C->SetRelativeScale3D(Size/100);
        C->SetRelativeRotation(R);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);
        C->RegisterComponent();
        DemoParts.Add(C);
        if (Group==1) SpanDetails.Add(C);
        if (Group==2) DebrisDetails.Add(C);
    };
    for (float Side : {-1.f,1.f})
    {
        Part(FVector(Side*80,0,-15),FVector(22,155,44),Concrete,0);
        Part(FVector(0,Side*66,-14),FVector(176,9,19),Steel,1);
        Part(FVector(0,Side*71,45),FVector(178,5,5),Steel,1);
        Part(FVector(0,Side*71,26),FVector(178,4,4),Steel,1);
        Part(FVector(0,Side*71,49),FVector(178,7,3),Snow,1);
        for (float X : {-80.f,-40.f,0.f,40.f,80.f})
            Part(FVector(X,Side*71,27),FVector(6,6,38),Steel,1);
        for (int32 I=0; I<3; ++I)
            Part(FVector(Side*(60+I*9),-45+I*42,4+I*3),FVector(23,30,16),Concrete,2,FRotator(12*I,Side*23,Side*18));
        Part(FVector(Side*70,Side*56,18),FVector(38,5,5),Steel,2,FRotator(Side*24,Side*18,0));
    }
    Part(FVector(0,0,12),FVector(176,124,2),Snow,1);
    ApplyState();
}

bool AOKViaductActor::IsDemoModelConsistent() const
{
    if (SpanDetails.IsEmpty() || DebrisDetails.IsEmpty()) return false;
    for (UStaticMeshComponent* Part : SpanDetails) if (Part->IsVisible()==bDestroyed) return false;
    for (UStaticMeshComponent* Part : DebrisDetails) if (Part->IsVisible()!=bDestroyed) return false;
    return IntactSpan->IsVisible()!=bDestroyed;
}
