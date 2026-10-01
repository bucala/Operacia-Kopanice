#include "ECS/OKTacticalCharacter.h"
#include "ECS/OKEntityComponents.h"
#include "Environment/OKTerrainMovementSurfaceComponent.h"

AOKTacticalCharacter::AOKTacticalCharacter()
{
    Entity = CreateDefaultSubobject<UOKEntityStateComponent>(TEXT("Entity"));
    Terrain = CreateDefaultSubobject<UOKTerrainMovementSurfaceComponent>(TEXT("Terrain"));
}
