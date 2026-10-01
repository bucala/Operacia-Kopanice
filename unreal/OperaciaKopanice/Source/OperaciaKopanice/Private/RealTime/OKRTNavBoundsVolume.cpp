#include "RealTime/OKRTNavBoundsVolume.h"
#include "Components/BoxComponent.h"

AOKRTNavBoundsVolume::AOKRTNavBoundsVolume()
{
    Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("RuntimeBounds"));
    Bounds->SetupAttachment(GetRootComponent());
    Bounds->SetBoxExtent(FVector(1100,1000,500));
    Bounds->SetCollisionProfileName(TEXT("NoCollision"));
    Bounds->SetCanEverAffectNavigation(false);
}
