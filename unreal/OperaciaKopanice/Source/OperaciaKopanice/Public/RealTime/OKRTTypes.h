#pragma once
#include "CoreMinimal.h"
#include "OKRTTypes.generated.h"

class AOKRTUnit;

UENUM(BlueprintType)
enum class EOKStance : uint8 { Walk, Run, Crouch, Prone };
UENUM(BlueprintType)
enum class EOKAlert : uint8 { Patrol, Investigate, Combat };
UENUM(BlueprintType)
enum class EOKOrder : uint8 { Move, Interact, Takedown, Distract, Carry, Stance, Heal, Knockout, Bind };
UENUM(BlueprintType)
enum class EOKSpecialist : uint8 { Partisan, Officer };
UENUM(BlueprintType)
enum class EOKInteraction : uint8 { Nearby, CollectTNT, DetonateBridge, HideBody, CollectDocuments, SabotageCommandPost };
UENUM(BlueprintType)
enum class EOKGuardDuty : uint8 { Patrol, HoldPost };

USTRUCT(BlueprintType)
struct FOKRTPatrolStop
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Location=FVector::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float FacingYaw=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float WaitSeconds=1;
    FOKRTPatrolStop()=default;
    FOKRTPatrolStop(FVector Position,float Yaw,float Duration)
        : Location(Position),FacingYaw(Yaw),WaitSeconds(Duration) {}
};

USTRUCT(BlueprintType)
struct FOKRTOrder
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EOKOrder Kind=EOKOrder::Move;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Location=FVector::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TWeakObjectPtr<AOKRTUnit> Target;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EOKStance Stance=EOKStance::Walk;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRunToDestination=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bApproachInteraction=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EOKInteraction Interaction=EOKInteraction::Nearby;
    UPROPERTY(BlueprintReadOnly) int32 SyncBatch=0;
};
