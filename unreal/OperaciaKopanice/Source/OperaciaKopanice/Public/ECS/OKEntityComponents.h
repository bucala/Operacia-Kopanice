#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OKEntityComponents.generated.h"

UENUM(BlueprintType)
enum class EOKFaction : uint8 { Partisan, Volksgrenadier708, Civilian };

UCLASS(ClassGroup=(OperaciaKopanice), meta=(BlueprintSpawnableComponent))
class OPERACIAKOPANICE_API UOKEntityStateComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOKEntityStateComponent() { PrimaryComponentTick.bCanEverTick = false; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Entity")
    FName EntityId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Entity")
    EOKFaction Faction = EOKFaction::Partisan;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Entity")
    FIntPoint GridPosition = FIntPoint::ZeroValue;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Entity")
    int32 Health = 100;
};
