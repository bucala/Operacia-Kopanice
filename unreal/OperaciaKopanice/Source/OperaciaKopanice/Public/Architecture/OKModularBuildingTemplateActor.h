#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OKModularBuildingTemplateActor.generated.h"

UENUM(BlueprintType)
enum class EOKModularBuildingStyle : uint8
{
	DrevenicaLogCabin,
	CarpathianStonework,
	MixedLogAndStone
};

UCLASS()
class OPERACIAKOPANICE_API AOKModularBuildingTemplateActor : public AActor
{
	GENERATED_BODY()

public:
	AOKModularBuildingTemplateActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	EOKModularBuildingStyle Style = EOKModularBuildingStyle::DrevenicaLogCabin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BayCountX = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "1", ClampMax = "6"))
	int32 BayCountY = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "180.0"))
	float BaySize = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "160.0"))
	float WallHeight = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	bool bGeneratePorch = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> LogMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> StoneMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> RoofMaterial = nullptr;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Building")
	void RebuildTemplate();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> GeneratedParts;

	void ClearGeneratedParts();
	UStaticMeshComponent* AddBoxPart(FName Name, FVector LocalLocation, FVector LocalScale, UMaterialInterface* Material);
};
