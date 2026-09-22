#include "Navigation/OKDynamicNavObstacleComponent.h"

#include "NavigationSystem.h"

UOKDynamicNavObstacleComponent::UOKDynamicNavObstacleComponent()
{
	SetCollisionProfileName(TEXT("BlockAll"));
	SetCanEverAffectNavigation(true);
	bDynamicObstacle = true;
}

void UOKDynamicNavObstacleComponent::BeginPlay()
{
	Super::BeginPlay();
	SetObstacleActive(bStartsBlocked);
}

void UOKDynamicNavObstacleComponent::SetObstacleActive(bool bActive)
{
	SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(bActive);
	MarkNavigationDirty();
}

void UOKDynamicNavObstacleComponent::MarkNavigationDirty()
{
	if (!GetWorld())
	{
		return;
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSystem->UpdateComponentInNavOctree(*this);
	}
}
