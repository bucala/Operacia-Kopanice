#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "OKRTVisionComponent.generated.h"

class AOKRTUnit;

UCLASS(ClassGroup=(Kopanice),meta=(BlueprintSpawnableComponent))
class OPERACIAKOPANICE_API UOKRTVisionComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    UOKRTVisionComponent(const FObjectInitializer& Initializer=FObjectInitializer::Get());
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float NearRange=330.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float FarRange=1000.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float HalfAngle=45.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bConeVisible=false;
    bool Sees(const AOKRTUnit* Target,bool& bPrimary) const;
    void Refresh();
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
private:
    float Accumulator=0.f;
};
