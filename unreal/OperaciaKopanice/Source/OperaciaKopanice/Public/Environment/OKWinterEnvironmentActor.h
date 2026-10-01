#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OKWinterEnvironmentActor.generated.h"

class AStaticMeshActor;
class UStaticMesh;
class UMaterialInterface;

// Visual scenery only. Each game mode supplies its own collision and objectives.
UCLASS()
class OPERACIAKOPANICE_API AOKWinterEnvironmentActor : public AActor
{
    GENERATED_BODY()
public:
    AOKWinterEnvironmentActor();
    bool BuildEnvironment();
    void BuildWinterTree(FVector Origin, int32 Seed, float Height=285.f);
private:
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;
    AStaticMeshActor* Mesh(FVector Location,FVector Scale,FLinearColor Color,UStaticMesh* Asset=nullptr);
};
