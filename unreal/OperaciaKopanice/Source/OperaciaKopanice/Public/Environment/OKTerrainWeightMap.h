#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OKTerrainWeightMap.generated.h"

// CPU counterpart of the landscape paint masks, stored row-major, X varying fastest.
UCLASS(BlueprintType)
class OPERACIAKOPANICE_API UOKTerrainWeightMap : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weights")
    FIntPoint Resolution = FIntPoint(2, 2);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weights")
    FVector2D WorldOriginCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weights")
    FVector2D WorldSizeCm = FVector2D(10000, 10000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weights")
    TArray<FVector> Weights;

    UFUNCTION(BlueprintPure, Category="Weights")
    bool Sample(FVector WorldLocation, FVector& OutWeights) const;
};
