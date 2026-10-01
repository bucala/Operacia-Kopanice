#pragma once
#include "CoreMinimal.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "OKRTNavBoundsVolume.generated.h"

class UBoxComponent;

// A runtime-spawned volume has no editor-built brush; this box supplies real bounds.
UCLASS()
class OPERACIAKOPANICE_API AOKRTNavBoundsVolume : public ANavMeshBoundsVolume
{
    GENERATED_BODY()
public:
    AOKRTNavBoundsVolume();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Bounds;
};
