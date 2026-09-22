#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OKTacticalCharacter.generated.h"

class UOKEntityStateComponent;
class UOKTerrainMovementSurfaceComponent;

UCLASS()
class OPERACIAKOPANICE_API AOKTacticalCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AOKTacticalCharacter();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ECS")
    TObjectPtr<UOKEntityStateComponent> Entity;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ECS")
    TObjectPtr<UOKTerrainMovementSurfaceComponent> Terrain;
};
