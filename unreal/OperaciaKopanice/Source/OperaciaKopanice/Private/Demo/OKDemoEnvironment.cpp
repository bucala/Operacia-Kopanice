#include "Demo/OKDemoGameMode.h"
#include "Environment/OKWinterEnvironmentActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

void AOKDemoGameMode::BuildEnvironment()
{
    Environment=GetWorld()->SpawnActor<AOKWinterEnvironmentActor>();
    OverlayMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_TacticalOverlay.M_TacticalOverlay"));
    bContinuousTerrain=Environment->BuildEnvironment() && OverlayMaterial;
}
