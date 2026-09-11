#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "OKDynamicNavObstacleComponent.generated.h"

UCLASS(ClassGroup = (OperaciaKopanice), meta = (BlueprintSpawnableComponent))
class OPERACIAKOPANICE_API UOKDynamicNavObstacleComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UOKDynamicNavObstacleComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	bool bStartsBlocked = false;

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void SetObstacleActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void MarkNavigationDirty();

protected:
	virtual void BeginPlay() override;
};
