#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OKTurnCoordinatorSubsystem.generated.h"

UENUM(BlueprintType)
enum class EOKTurnPhase : uint8 { PlayerAction, EnemyReaction, OutcomeEvaluation };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOKPhaseChanged, EOKTurnPhase, Phase, int32, TurnNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOKTurnsReset);

UCLASS()
class OPERACIAKOPANICE_API UOKTurnCoordinatorSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Turns")
    EOKTurnPhase Phase = EOKTurnPhase::PlayerAction;
    UPROPERTY(BlueprintReadOnly, Category="Turns")
    int32 TurnNumber = 0;
    UPROPERTY(BlueprintAssignable, Category="Turns")
    FOKPhaseChanged OnPhaseChanged;
    UPROPERTY(BlueprintAssignable, Category="Turns")
    FOKTurnsReset OnReset;

    UFUNCTION(BlueprintCallable, Category="Turns")
    bool CommitPlayerAction();
    UFUNCTION(BlueprintCallable, Category="Turns")
    bool CompleteEnemyReaction();
    UFUNCTION(BlueprintCallable, Category="Turns")
    bool CompleteOutcomeEvaluation();
    UFUNCTION(BlueprintCallable, Category="Turns")
    void ResetTurns();
private:
    bool bBroadcasting = false;
    bool Transition(EOKTurnPhase Expected, EOKTurnPhase Next);
};
