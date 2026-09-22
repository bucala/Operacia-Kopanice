#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Demo/OKDemoRules.h"
#include "OKDemoGameMode.generated.h"

class AStaticMeshActor;
class ACameraActor;
class AOKViaductActor;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class OPERACIAKOPANICE_API AOKDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AOKDemoGameMode();
    virtual void StartPlay() override;
    void Act(OKDemo::EAction Action);
    void ClickCell(FIntPoint Cell);
    void Restart();
    void Undo();
    void SelectLevel(int32 Index);
    void ToggleMenu();
    void SetMenuPage(int32 Page) { MenuPage=FMath::Clamp(Page,0,3); }
    int32 GetMenuPage() const { return MenuPage; }
    void ToggleGrid();
    void ToggleCamera();
    void ToggleRotationMode();
    void CycleQuality();
    void Orbit(float YawDelta, float TiltDelta, float ZoomDelta = 0);
    void ResetCamera();
    bool IsGridVisible() const { return bGridVisible; }
    bool IsCameraEnabled() const { return bCameraEnabled; }
    bool IsSteppedRotation() const { return bSteppedRotation; }
    int32 GetQuality() const { return Quality; }
    FString Objective() const;
    const OKDemo::FState& GetState() const { return State; }
    const FString& GetMessage() const { return Message; }
    FVector CellLocation(FIntPoint Cell, float Z = 0) const;
private:
    OKDemo::FState State;
    TArray<OKDemo::FState> History;
    FString Message;
    int32 MenuPage = 0;
    bool bGridVisible = true;
    bool bCameraEnabled = true;
    bool bSteppedRotation = true;
    int32 Quality = 2;
    float CameraYaw = 58.86f;
    float CameraTilt = 51.87f;
    float CameraDistance = 4450.f;
    void SaveOptions();
    void UpdateCamera();
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> OverlayMaterial;
    bool bContinuousTerrain = false;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Tiles;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Guards;
    UPROPERTY() TObjectPtr<AStaticMeshActor> PlayerMarker;
    UPROPERTY() TObjectPtr<AStaticMeshActor> TNTMarker;
    UPROPERTY() TObjectPtr<AStaticMeshActor> DetonatorMarker;
    UPROPERTY() TObjectPtr<AStaticMeshActor> ExitMarker;
    UPROPERTY() TObjectPtr<AOKViaductActor> Bridge;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    void BuildScene();
    void BuildEnvironment();
    void BuildWinterTree(FVector Origin, int32 Seed, float Height = 285.f);
    AStaticMeshActor* BuildUnit(bool bGuard);
    void SetupSmokeTest();
    void RefreshScene();
    void SyncTurns();
    void Tint(AStaticMeshActor* Actor, FLinearColor Color);
    AStaticMeshActor* Mesh(FVector Location, FVector Scale, FLinearColor Color, UStaticMesh* Asset = nullptr);
};
