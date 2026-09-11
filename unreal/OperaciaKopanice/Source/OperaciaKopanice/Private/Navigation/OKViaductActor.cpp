#include "Navigation/OKViaductActor.h"
#include "Navigation/OKDynamicNavObstacleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "NavigationSystem.h"
#include "NavAreas/NavArea_Null.h"

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
