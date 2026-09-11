#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OKConcreteTilePlatformActor.generated.h"

class UTextRenderComponent;

UCLASS()
class OPERACIAKOPANICE_API AOKConcreteTilePlatformActor : public AActor
{
	GENERATED_BODY()

public:
	AOKConcreteTilePlatformActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FString SideEngraving = TEXT("TATRA T77A / 1938");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FString RearEngraving = TEXT("OPERACIA KOPANICE");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FVector TileDimensions = FVector(460.0f, 260.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	TObjectPtr<UMaterialInterface> ConcreteMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	TObjectPtr<UMaterialInterface> EngravingTextMaterial = nullptr;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Platform")
	void RefreshEngravings();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TileMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTextRenderComponent> LeftEngravingText = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTextRenderComponent> RightEngravingText = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTextRenderComponent> RearEngravingText = nullptr;
};
