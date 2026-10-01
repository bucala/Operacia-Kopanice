#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "RealTime/OKRTTypes.h"
#include "OKRTGuardController.generated.h"

class UAIPerceptionComponent;
class AOKRTUnit;

// Party controllers use path following only; guard controllers additionally run the FSM.
UCLASS()
class OPERACIAKOPANICE_API AOKRTGuardController : public AAIController
{
    GENERATED_BODY()
public:
    AOKRTGuardController();
    virtual void Tick(float Delta) override;
    virtual FGenericTeamId GetGenericTeamId() const override;
    void Investigate(FVector Location);
    void Alert(AOKRTUnit* Target,bool bBroadcast=true);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAIPerceptionComponent> Senses;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FVector> PatrolRoute;
    UPROPERTY(BlueprintReadOnly) EOKAlert AlertState=EOKAlert::Patrol;
    UPROPERTY(BlueprintReadOnly) float Suspicion=0;
    UPROPERTY(BlueprintReadOnly) FVector LastKnown=FVector::ZeroVector;
    bool bBrainEnabled=true;
protected:
    UFUNCTION() void Perceived(AActor* Actor,FAIStimulus Stimulus);
private:
    UPROPERTY() TWeakObjectPtr<AOKRTUnit> CombatTarget;
    UPROPERTY() TWeakObjectPtr<AOKRTUnit> PursuitTarget;
    TSet<TWeakObjectPtr<AOKRTUnit>> DiscoveredBodies;
    int32 PatrolIndex=0;
    float Wait=0;
    float LostSight=0;
    float ShotClock=0;
    float MoveRetry=0;
};
