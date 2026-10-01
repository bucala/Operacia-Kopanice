#pragma once

#include "CoreMinimal.h"
#include "Elements/Blueprint/PCGBlueprintBaseElement.h"
#include "Environment/OKPCGScatterRuleSet.h"
#include "OKPCGScatterElement.generated.h"

UCLASS(BlueprintType, Blueprintable)
class OPERACIAKOPANICE_API UOKPCGScatterElement : public UPCGBlueprintBaseElement
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scatter")
    TObjectPtr<UOKPCGScatterRuleSet> RuleSet;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scatter")
    int32 RuleIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scatter")
    int32 Seed = 1944;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scatter")
    float AltitudeOffsetMeters = 400.0f;
    virtual void Execute_Implementation(const FPCGDataCollection& Input, FPCGDataCollection& Output) override;
};
