#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Environment/OKTerrainSurfaceTypes.h"
#include "OKTerrainMovementSurfaceComponent.generated.h"

class UCharacterMovementComponent;
class UOKTerrainWeightMap;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOKTerrainSurfaceChangedSignature, EOKTerrainSurface, Surface, float, SpeedModifier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOKFootprintRequestedSignature, FVector, Location, FRotator, Rotation, EOKTerrainSurface, Surface);

UCLASS(ClassGroup = (OperaciaKopanice), meta = (BlueprintSpawnableComponent))
class OPERACIAKOPANICE_API UOKTerrainMovementSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOKTerrainMovementSurfaceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UOKTerrainSurfaceDataAsset> SurfaceData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UOKTerrainWeightMap> WeightMap = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints")
	TObjectPtr<UMaterialInterface> MudDecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float TraceDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bApplySpeedModifierToCharacterMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0"))
	float BaseWalkSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints", meta = (ClampMin = "0.0"))
	float FootstepSpacing = 72.0f;

	UPROPERTY(BlueprintAssignable, Category = "Terrain")
	FOKTerrainSurfaceChangedSignature OnTerrainSurfaceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Footprints")
	FOKFootprintRequestedSignature OnFootprintRequested;

	UFUNCTION(BlueprintPure, Category = "Terrain")
	EOKTerrainSurface GetCurrentSurface() const { return CurrentRuntime.Surface; }

	UFUNCTION(BlueprintPure, Category = "Terrain")
	float GetCurrentSpeedModifier() const { return CurrentRuntime.SpeedModifier; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SampleSurface();
	void ApplyMovementModifier() const;
	void MaybeRequestFootprint(const FVector& Location);

	FOKTerrainSurfaceRuntime CurrentRuntime;
	FVector LastFootprintLocation = FVector::ZeroVector;
	bool bHasGroundSample = false;
	float BaseGroundFriction = 8.0f;
	TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;
};
