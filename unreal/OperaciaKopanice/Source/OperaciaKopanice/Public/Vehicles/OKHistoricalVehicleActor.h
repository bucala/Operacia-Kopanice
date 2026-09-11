#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OKHistoricalVehicleActor.generated.h"

UENUM(BlueprintType)
enum class EOKHistoricalVehicleType : uint8
{
	KubelwagenTyp82 UMETA(DisplayName = "Kubelwagen Typ 82"),
	OpelBlitz3T UMETA(DisplayName = "Opel Blitz 3t"),
	TatraT77A1938 UMETA(DisplayName = "Tatra T77A Limousine 1938")
};

UCLASS()
class OPERACIAKOPANICE_API AOKHistoricalVehicleActor : public AActor
{
	GENERATED_BODY()

public:
	AOKHistoricalVehicleActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	EOKHistoricalVehicleType VehicleType = EOKHistoricalVehicleType::TatraT77A1938;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<UStaticMesh> VehicleMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<UMaterialInterface> BodyMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<UMaterialInterface> GlassMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle", meta = (ClampMin = "0"))
	int32 GlassMaterialSlot = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MudAmount = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FrostAmount = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoofSnowAmount = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PaintWearAmount = 0.12f;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Vehicle")
	void ApplyVehiclePreset();

	UFUNCTION(BlueprintCallable, Category = "Weathering")
	void UpdateWeatheringMaterialParameters();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};
