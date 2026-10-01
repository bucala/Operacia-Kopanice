#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OKFootprintFieldActor.generated.h"

class UTextureRenderTarget2D;
class UMaterialInterface;
class ALandscapeProxy;

UCLASS()
class OPERACIAKOPANICE_API AOKFootprintFieldActor : public AActor
{
    GENERATED_BODY()
public:
    AOKFootprintFieldActor();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footprints")
    TObjectPtr<UTextureRenderTarget2D> DepthTarget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footprints")
    TObjectPtr<UMaterialInterface> StampMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footprints")
    TObjectPtr<ALandscapeProxy> Landscape;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footprints", meta=(ClampMin="100"))
    float FieldSizeCm = 10000.0f;
    UFUNCTION(BlueprintCallable, Category="Footprints")
    void RefreshFootprints();
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
};
