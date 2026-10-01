#pragma once
#include "CoreMinimal.h"
#include "OKRTTypes.generated.h"

class AOKRTUnit;

UENUM(BlueprintType)
enum class EOKStance : uint8 { Walk, Run, Crouch, Prone };
UENUM(BlueprintType)
enum class EOKAlert : uint8 { Patrol, Investigate, Combat };
UENUM(BlueprintType)
enum class EOKOrder : uint8 { Move, Interact, Takedown, Distract, Carry, Stance };

USTRUCT(BlueprintType)
struct FOKRTOrder
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EOKOrder Kind=EOKOrder::Move;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Location=FVector::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TWeakObjectPtr<AOKRTUnit> Target;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EOKStance Stance=EOKStance::Walk;
};
