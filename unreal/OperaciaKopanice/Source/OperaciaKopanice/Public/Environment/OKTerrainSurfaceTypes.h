#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "OKTerrainSurfaceTypes.generated.h"

UENUM(BlueprintType)
enum class EOKTerrainSurface : uint8
{
	PavedRoadsInteriors UMETA(DisplayName = "Paved Roads / Interiors"),
	AutumnMud UMETA(DisplayName = "Autumn Mud"),
	DeepWinterSnow UMETA(DisplayName = "Deep Winter Snow")
};

USTRUCT(BlueprintType)
struct FOKTerrainSurfaceRuntime
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	EOKTerrainSurface Surface = EOKTerrainSurface::PavedRoadsInteriors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	FName LandscapeLayerName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "0.0"))
	float Friction = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "0.0"))
	float SpeedModifier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UPhysicalMaterial> PhysicalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints")
	bool bSpawnFootprints = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints")
	bool bFootprintsAreTemporary = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints")
	bool bFootprintsDetectableByAI = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints", meta = (EditCondition = "bSpawnFootprints", ClampMin = "0.0"))
	float FootprintLifetimeSeconds = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints", meta = (ClampMin = "1"))
	int32 FootprintLifetimeTurns = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints", meta = (EditCondition = "bFootprintsDetectableByAI", ClampMin = "0.0"))
	float AISnowTrackSenseRadius = 900.0f;
};

UCLASS(BlueprintType)
class OPERACIAKOPANICE_API UOKTerrainSurfaceDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UOKTerrainSurfaceDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	TArray<FOKTerrainSurfaceRuntime> Surfaces;

	UFUNCTION(BlueprintPure, Category = "Terrain")
	FOKTerrainSurfaceRuntime GetRuntimeForSurface(EOKTerrainSurface Surface) const;

	UFUNCTION(BlueprintPure, Category = "Terrain")
	FOKTerrainSurfaceRuntime GetRuntimeForPhysicalMaterial(const UPhysicalMaterial* PhysicalMaterial) const;

	// X = paved, Y = mud, Z = snow. Physics traces only return the dominant layer.
	UFUNCTION(BlueprintPure, Category = "Terrain")
	FOKTerrainSurfaceRuntime BlendRuntime(FVector Weights) const;
};
