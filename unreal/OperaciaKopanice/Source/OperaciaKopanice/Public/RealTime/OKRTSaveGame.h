#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RealTime/OKRTTypes.h"
#include "OKRTSaveGame.generated.h"

USTRUCT()
struct FOKSavedOrder
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) EOKOrder Kind=EOKOrder::Move;
    UPROPERTY(SaveGame) FVector Location=FVector::ZeroVector;
    UPROPERTY(SaveGame) int32 Target=INDEX_NONE;
    UPROPERTY(SaveGame) EOKStance Stance=EOKStance::Walk;
    UPROPERTY(SaveGame) bool bRun=false;
    UPROPERTY(SaveGame) bool bApproach=false;
    UPROPERTY(SaveGame) EOKInteraction Interaction=EOKInteraction::Nearby;
    UPROPERTY(SaveGame) int32 Batch=0;
};
USTRUCT()
struct FOKSavedUnit
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FVector Feet=FVector::ZeroVector;
    UPROPERTY(SaveGame) float Yaw=0;
    UPROPERTY(SaveGame) float Health=100;
    UPROPERTY(SaveGame) float Cooldown=0;
    UPROPERTY(SaveGame) int32 Distractions=5;
    UPROPERTY(SaveGame) int32 Medkits=0;
    UPROPERTY(SaveGame) EOKStance Stance=EOKStance::Walk;
    UPROPERTY(SaveGame) bool bSelected=false;
    UPROPERTY(SaveGame) bool bHidden=false;
    UPROPERTY(SaveGame) bool bCone=true;
    UPROPERTY(SaveGame) bool bUnconscious=false;
    UPROPERTY(SaveGame) bool bBound=false;
    UPROPERTY(SaveGame) float RecoveryRemaining=0;
    UPROPERTY(SaveGame) int32 CarriedBody=INDEX_NONE;
    UPROPERTY(SaveGame) TArray<FOKSavedOrder> Orders;
};
USTRUCT()
struct FOKSavedGuard
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) EOKAlert Alert=EOKAlert::Patrol;
    UPROPERTY(SaveGame) float Suspicion=0;
    UPROPERTY(SaveGame) FVector LastKnown=FVector::ZeroVector;
    UPROPERTY(SaveGame) int32 PatrolIndex=0;
    UPROPERTY(SaveGame) bool bAtStop=false;
    UPROPERTY(SaveGame) float Wait=0;
    UPROPERTY(SaveGame) float LostSight=0;
    UPROPERTY(SaveGame) float ShotClock=0;
    UPROPERTY(SaveGame) int32 CombatTarget=INDEX_NONE;
    UPROPERTY(SaveGame) int32 RescueTarget=INDEX_NONE;
    UPROPERTY(SaveGame) TArray<int32> DiscoveredBodies;
};
UCLASS()
class OPERACIAKOPANICE_API UOKRTSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version=2;
    UPROPERTY(SaveGame) int32 MissionId=0;
    UPROPERTY(SaveGame) FDateTime SavedAt;
    UPROPERTY(SaveGame) bool bTNT=false;
    UPROPERTY(SaveGame) bool bBridge=false;
    UPROPERTY(SaveGame) bool bObjective=false;
    UPROPERTY(SaveGame) bool bPaused=false;
    UPROPERTY(SaveGame) bool bCommandMode=false;
    UPROPERTY(SaveGame) bool bPriorPause=false;
    UPROPERTY(SaveGame) int32 ActiveMember=0;
    UPROPERTY(SaveGame) int32 CompletedMissions=0;
    UPROPERTY(SaveGame) FVector CameraTarget=FVector::ZeroVector;
    UPROPERTY(SaveGame) float CameraYaw=0;
    UPROPERTY(SaveGame) float CameraTilt=47;
    UPROPERTY(SaveGame) float CameraDistance=4250;
    UPROPERTY(SaveGame) int32 ActiveBatch=0;
    UPROPERTY(SaveGame) int32 NextBatch=1;
    UPROPERTY(SaveGame) bool bReleased=false;
    UPROPERTY(SaveGame) TArray<bool> Participants;
    UPROPERTY(SaveGame) TArray<FOKSavedOrder> Drafts;
    UPROPERTY(SaveGame) TArray<bool> DraftValid;
    UPROPERTY(SaveGame) TArray<FOKSavedUnit> Units;
    UPROPERTY(SaveGame) TArray<FOKSavedGuard> Guards;
};
