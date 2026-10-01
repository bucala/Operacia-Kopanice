#include "AI/OKWeatherPerceptionComponent.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Actor.h"

UOKWeatherPerceptionComponent::UOKWeatherPerceptionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOKWeatherPerceptionComponent::ApplyWeatherState(EOKWeatherVisibilityState NewWeatherState, float NewFogDensity, float NewSnowfallIntensity)
{
	WeatherState = NewWeatherState;
	FogDensity = FMath::Clamp(NewFogDensity, 0.0f, 1.0f);
	SnowfallIntensity = FMath::Clamp(NewSnowfallIntensity, 0.0f, 1.0f);
	PushToAIPerceptionComponent();
	OnWeatherPerceptionChanged.Broadcast(GetSightRangeMultiplier(), GetHearingRangeMultiplier());
}

float UOKWeatherPerceptionComponent::GetSightRangeMultiplier() const
{
	float Multiplier = 1.0f;
	if (WeatherState == EOKWeatherVisibilityState::AutumnFog)
	{
		Multiplier *= FMath::Lerp(1.0f, 0.42f, FogDensity);
	}
	else if (WeatherState == EOKWeatherVisibilityState::WinterSnowfall)
	{
		Multiplier *= FMath::Lerp(1.0f, 0.62f, SnowfallIntensity);
	}
	else if (WeatherState == EOKWeatherVisibilityState::Blizzard)
	{
		Multiplier *= FMath::Lerp(0.58f, 0.24f, FMath::Max(FogDensity, SnowfallIntensity));
	}

	return Multiplier;
}

float UOKWeatherPerceptionComponent::GetHearingRangeMultiplier() const
{
	float Multiplier = 1.0f;
	if (WeatherState == EOKWeatherVisibilityState::WinterSnowfall)
	{
		Multiplier *= FMath::Lerp(1.0f, 0.72f, SnowfallIntensity);
	}
	if (WeatherState == EOKWeatherVisibilityState::Blizzard)
	{
		Multiplier *= 0.58f;
	}

	return Multiplier;
}

void UOKWeatherPerceptionComponent::PushToAIPerceptionComponent()
{
	UAIPerceptionComponent* Perception = GetOwner() ? GetOwner()->FindComponentByClass<UAIPerceptionComponent>() : nullptr;
	if (!Perception)
	{
		return;
	}

	for (auto It = Perception->GetSensesConfigIterator(); It; ++It)
	{
		UAISenseConfig* Config = *It;
		if (UAISenseConfig_Sight* Sight = Cast<UAISenseConfig_Sight>(Config))
		{
			Sight->SightRadius = BaseSightRange * GetSightRangeMultiplier();
			Sight->LoseSightRadius = Sight->SightRadius * 1.16f;
			Sight->PeripheralVisionAngleDegrees = BaseSightHalfAngle * FMath::Lerp(0.65f, 1.0f, GetSightRangeMultiplier());
		}
		else if (UAISenseConfig_Hearing* Hearing = Cast<UAISenseConfig_Hearing>(Config))
		{
			Hearing->HearingRange = BaseHearingRange * GetHearingRangeMultiplier();
		}
		Perception->ConfigureSense(*Config);
	}

	Perception->RequestStimuliListenerUpdate();
}
