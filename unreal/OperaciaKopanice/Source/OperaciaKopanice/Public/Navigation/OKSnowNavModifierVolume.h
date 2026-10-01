#pragma once
#include "CoreMinimal.h"
#include "NavModifierVolume.h"
#include "OKSnowNavModifierVolume.generated.h"

UCLASS()
class OPERACIAKOPANICE_API AOKSnowNavModifierVolume : public ANavModifierVolume
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Snow")
    float SnowDepthCm = 35.0f;
    UFUNCTION(BlueprintCallable, Category="Snow")
    void SetSnowDepth(float DepthCm);
    virtual void OnConstruction(const FTransform& Transform) override;
};
