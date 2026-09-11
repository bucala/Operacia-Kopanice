#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OKViaductActor.generated.h"

class UStaticMeshComponent;
class UOKDynamicNavObstacleComponent;

UCLASS()
class OPERACIAKOPANICE_API AOKViaductActor : public AActor
{
    GENERATED_BODY()
public:
    AOKViaductActor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bridge")
    TObjectPtr<UStaticMeshComponent> IntactSpan;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bridge")
    TObjectPtr<UStaticMeshComponent> Rubble;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bridge")
    TObjectPtr<UOKDynamicNavObstacleComponent> GapBlocker;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bridge")
    bool bDestroyed = false;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Bridge")
    void DestroySpan();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Bridge")
    void RepairSpan();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    virtual void BeginPlay() override;
private:
    void ApplyState();
};
