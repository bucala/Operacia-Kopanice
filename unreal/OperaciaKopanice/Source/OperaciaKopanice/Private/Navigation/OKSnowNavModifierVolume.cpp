#include "Navigation/OKSnowNavModifierVolume.h"
#include "Navigation/OKSnowDepthNavArea.h"
#include "NavAreas/NavArea_Default.h"

void AOKSnowNavModifierVolume::SetSnowDepth(float DepthCm)
{
    SnowDepthCm = FMath::Max(0.0f, DepthCm);
    SetAreaClass(SnowDepthCm >= 20.0f ? UOKSnowDepthNavArea::StaticClass() : UNavArea_Default::StaticClass());
}

void AOKSnowNavModifierVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SetSnowDepth(SnowDepthCm);
}
