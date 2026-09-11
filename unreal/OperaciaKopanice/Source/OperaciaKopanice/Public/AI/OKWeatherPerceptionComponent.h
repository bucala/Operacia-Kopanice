#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OKWeatherPerceptionComponent.generated.h"

UENUM(BlueprintType)
enum class EOKWeatherVisibilityState : uint8
{
	Clear,
	AutumnFog,
	WinterSnowfall,
	Blizzard
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOKWeatherPerceptionChangedSignature, float, SightRangeMultiplier, float, HearingRangeMultiplier);

UCLASS(ClassGroup = (OperaciaKopanice), meta = (BlueprintSpawnableComponent))
class OPERACIAKOPANICE_API UOKWeatherPerceptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOKWeatherPerceptionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EOKWeatherVisibilityState WeatherState = EOKWeatherVisibilityState::Clear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogDensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SnowfallIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float BaseSightRange = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "1", ClampMax = "89"))
	float BaseSightHalfAngle = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float BaseHearingRange = 1600.0f;

	UPROPERTY(BlueprintAssignable, Category = "Weather")
	FOKWeatherPerceptionChangedSignature OnWeatherPerceptionChanged;

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void ApplyWeatherState(EOKWeatherVisibilityState NewWeatherState, float NewFogDensity, float NewSnowfallIntensity);

	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetSightRangeMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetHearingRangeMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void PushToAIPerceptionComponent();
};
