#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RealTime/OKRTTypes.h"
#include "OKRTUnit.generated.h"

class UOKRTVisionComponent;
class UStaticMeshComponent;
class UPoseableMeshComponent;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class OPERACIAKOPANICE_API AOKRTUnit : public ACharacter
{
    GENERATED_BODY()
public:
    AOKRTUnit();
    virtual void Tick(float Delta) override;
    virtual void GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const override;
    void Initialize(bool bGuard,bool bOfficer);
    UFUNCTION(BlueprintCallable) bool Submit(const FOKRTOrder& Order,bool bAppend=false);
    bool PromoteLastMoveToRun(FVector Destination);
    UFUNCTION(BlueprintCallable) void CancelOrders();
    UFUNCTION(BlueprintCallable) void SetStance(EOKStance Value);
    void TakeHit(float Damage);
    void DropBody(bool bHide=false);
    bool IsAlive() const { return Health>0; }
    float VisibilityFactor() const;
    FVector Feet() const;
    int32 QueueSize() const { return Orders.Num(); }
    bool GetQueuedOrder(int32 Index,FOKRTOrder& Order) const;
    bool HasStartedOrder() const;
    bool CanUndoLastOrder() const;
    UFUNCTION(BlueprintCallable) bool UndoLastOrder();
    void GetQueuedLocations(TArray<FVector>& Locations) const;
    bool IsSelected() const { return bSelected; }
    void Select(bool bValue) { bSelected=bValue; }
    UPROPERTY(BlueprintReadOnly) bool bEnemy=false;
    UPROPERTY(BlueprintReadOnly) float Health=100;
    UPROPERTY(BlueprintReadOnly) EOKStance Stance=EOKStance::Walk;
    UPROPERTY(BlueprintReadOnly) int32 Distractions=5;
    UPROPERTY(BlueprintReadOnly) float Cooldown=0;
    UPROPERTY(BlueprintReadOnly) bool bInCover=false;
    UPROPERTY(BlueprintReadOnly) bool bHiddenBody=false;
    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPoseableMeshComponent> AnimatedVisual;
    float GetVisualHeight() const { return VisualHeight; }
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UOKRTVisionComponent> Vision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAIPerceptionStimuliSourceComponent> Stimuli;
    UPROPERTY() TWeakObjectPtr<AOKRTUnit> CarriedBody;
    UPROPERTY() TWeakObjectPtr<AOKRTUnit> Carrier;
private:
    UPROPERTY() TArray<FOKRTOrder> Orders;
    bool bSelected=false;
    bool bOrderStarted=false;
    float NoiseClock=0;
    float VisualHeight=180;
    float GaitPhase=0;
    float SmoothedSpeed=0;
    float MotionBlend=0;
    float CrouchBlend=0;
    float ProneBlend=0;
    void UpdateAnimation(float Delta);
    void FinishOrder();
    bool ValidateOrder(const FOKRTOrder& Order,bool bExecuting,bool bKeepQueue,FString& Reason) const;
    bool CanNavigateOrder(const FOKRTOrder& Order,bool bFromQueue,FString& Reason);
    void UpdateOrders();
};
