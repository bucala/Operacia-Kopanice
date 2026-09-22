#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OKPCGScatterRuleSet.generated.h"

UENUM(BlueprintType)
enum class EOKScatterPrototype : uint8
{
	EuropeanBeech,
	LimestoneVrsatecRock,
	UnderstoryShrub,
	FallenBranch
};

USTRUCT(BlueprintType)
struct OPERACIAKOPANICE_API FOKPCGScatterRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	EOKScatterPrototype Prototype = EOKScatterPrototype::EuropeanBeech;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG", meta = (ClampMin = "0.0"))
	float DensityPer1000SqM = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MinSlopeDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxSlopeDegrees = 38.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	float MinAltitude = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	float MaxAltitude = 970.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	FVector2D UniformScaleRange = FVector2D(0.85f, 1.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	bool bAlignToNormal = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	bool bEnableNaniteOnGeneratedMeshes = true;
};

UCLASS(BlueprintType)
class OPERACIAKOPANICE_API UOKPCGScatterRuleSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UOKPCGScatterRuleSet();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	TArray<FOKPCGScatterRule> Rules;

	UFUNCTION(BlueprintPure, Category="PCG")
	bool EvaluatePoint(int32 RuleIndex, FTransform SurfaceTransform, float InputDensity, int32 PointSeed,
		float AltitudeOffsetMeters, FTransform& OutTransform) const;
};
