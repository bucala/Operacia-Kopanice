#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Environment/OKTerrainSurfaceTypes.h"
#include "ECS/OKEntityComponents.h"
#include "OKFootprintTrackerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct OPERACIAKOPANICE_API FOKTrackedFootprint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	EOKTerrainSurface Surface = EOKTerrainSurface::PavedRoadsInteriors;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	TWeakObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	int32 SpawnTurn = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	int32 ExpireTurn = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	EOKFaction SourceFaction = EOKFaction::Civilian;

	UPROPERTY(BlueprintReadOnly, Category = "Footprint")
	bool bDetectableByAI = false;
};

UCLASS()
class OPERACIAKOPANICE_API UOKFootprintTrackerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Footprints")
	void RegisterFootprint(AActor* SourceActor, FVector Location, EOKTerrainSurface Surface, int32 LifetimeTurns, bool bDetectableByAI, FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Footprints")
	TArray<FOKTrackedFootprint> QueryDetectableFootprints(FVector Center, float Radius) const;

	UFUNCTION(BlueprintPure, Category = "Footprints")
	TArray<FOKTrackedFootprint> GetActiveFootprints() const;

	UFUNCTION(BlueprintCallable, Category = "Footprints")
	void ClearFootprints();

private:
	UPROPERTY()
	TArray<FOKTrackedFootprint> Footprints;
};
