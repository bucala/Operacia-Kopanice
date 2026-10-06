#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RealTime/OKRTTypes.h"
#include "OKRTGameMode.generated.h"

class AOKRTUnit;
class AOKViaductActor;
class ACameraActor;
class UBoxComponent;
class USplineComponent;
class AStaticMeshActor;

USTRUCT()
struct FOKNoisePulse
{
    GENERATED_BODY()
    FVector Location=FVector::ZeroVector;
    float Radius=0;
    float Age=0;
};

UCLASS()
class OPERACIAKOPANICE_API AOKRTGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AOKRTGameMode();
    virtual void StartPlay() override;
    virtual void Tick(float Delta) override;
    void Select(int32 Index,bool bAppend=false);
    void SelectAll();
    bool Command(FOKRTOrder Order,bool bAppend=false);
    void TogglePause();
    void ToggleMenu();
    void LoadPreferences();
    void SavePreferences();
    void ToggleCones();
    void CycleQuality();
    void Orbit(float Yaw,float Tilt,float Zoom=0);
    void Pan(float X,float Y);
    void FocusSelected();
    void Restart();
    void Noise(AOKRTUnit* Source,FVector Location,float Loudness,float Range,FName Tag);
    void Alarm(AOKRTUnit* Target,AOKRTUnit* Source);
    void Interact(AOKRTUnit* Unit,EOKInteraction Kind=EOKInteraction::Nearby);
    bool Distract(AOKRTUnit* Unit,FVector Destination);
    EOKInteraction FindInteractionAt(FVector PickedLocation,FVector& Destination) const;
    bool IsCover(FVector Location) const;
    void Preview(FVector Destination);
    bool HasCompletePath(FVector From,FVector To) const;
    FString Objective() const;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AOKRTUnit>> Party;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AOKRTUnit>> Enemies;
    UPROPERTY(BlueprintReadOnly) bool bTacticalPause=false;
    UPROPERTY(BlueprintReadOnly) bool bHasTNT=false;
    UPROPERTY(BlueprintReadOnly) bool bBridgeDestroyed=false;
    UPROPERTY(BlueprintReadOnly) bool bWon=false;
    UPROPERTY(BlueprintReadOnly) bool bLost=false;
    bool bMenu=false;
    bool bOptions=false;
    bool bCones=true;
    bool bPathPreview=true;
    bool bSteppedCamera=true;
    int32 Quality=2;
    int32 ActiveMember=0;
    FString Message;
    TArray<FOKNoisePulse> NoisePulses;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FVector TNTLocation=FVector(540,1260,0);
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FVector DetonatorLocation=FVector(1440,720,0);
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FVector ExitLocation=FVector(1620,180,0);
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FVector HideLocation=FVector(540,1440,0);
    UPROPERTY() TObjectPtr<USplineComponent> PreviewSpline;
private:
    UPROPERTY() TObjectPtr<AOKViaductActor> Bridge;
    UPROPERTY() TObjectPtr<UBoxComponent> BridgeFloor;
    UPROPERTY() TObjectPtr<AStaticMeshActor> TNTMarker;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    FVector CameraTarget=FVector(900,720,0);
    float CameraYaw=58.86f;
    float CameraTilt=47.f;
    float CameraDistance=4250.f;
    void BuildScene();
    UBoxComponent* Collider(FVector Center,FVector Extent,FName Tag);
    AStaticMeshActor* Place(const TCHAR* Path,FVector Location,float Scale=1);
    void UpdateCamera();
    int32 PartyMembersOnSafeBank() const;
    void SmokeTick();
    void MissionSmokeTick(float Delta);
    void SmokeSelectionTests();
    void SmokeControlsTests();
    bool bSmoke=false;
    bool bMissionSmoke=false;
    bool bMissionSawBridge=false;
    float MissionAwayTime=0;
    float MissionLongestAway=0;
    bool bInitialPausePending=true;
    int32 SmokeStage=0;
    double SmokeStarted=0;
    double StageStarted=0;
    FVector PausePosition;
    void Require(bool bCondition,const TCHAR* Label);
};
