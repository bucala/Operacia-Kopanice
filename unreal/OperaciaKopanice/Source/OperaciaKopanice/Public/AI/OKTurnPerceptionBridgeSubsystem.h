#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "ECS/OKEntityComponents.h"
#include "OKTurnPerceptionBridgeSubsystem.generated.h"

USTRUCT()
struct FOKTurnNoise
{
	GENERATED_BODY()
	FVector Location = FVector::ZeroVector;
	float Loudness = 1.0f;
	float MaxRange = 1800.0f;
	EOKFaction Faction = EOKFaction::Civilian;
	TWeakObjectPtr<AActor> Instigator;
};

UCLASS()
class OPERACIAKOPANICE_API UOKTurnPerceptionBridgeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	UFUNCTION(BlueprintCallable, Category = "Turn|AI")
	void ReportTurnSound(AActor* InstigatorActor, FVector Location, float Loudness, FName Tag = NAME_None);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn|AI")
	float DefaultNoiseMaxRange = 1800.0f;

private:
	TArray<FOKTurnNoise> PendingSounds;
	UFUNCTION()
	void HandlePhase(EOKTurnPhase Phase, int32 TurnNumber);
	UFUNCTION()
	void ResetSounds();
};
