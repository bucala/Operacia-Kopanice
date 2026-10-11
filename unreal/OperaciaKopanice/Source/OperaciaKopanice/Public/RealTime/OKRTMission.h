#pragma once
#include "CoreMinimal.h"
#include "RealTime/OKRTTypes.h"

enum class EOKMissionGoal : uint8 { Bridge, Documents, CommandPost };
enum class EOKMenuPage : uint8 { Pause, Missions, Briefing, Options, ConfirmRestart, ConfirmQuit };
enum class EOKObjectiveStage : uint8 { Supply, Crossing, Target, Extraction };

struct FOKRTMissionGuard
{
    FVector Spawn;
    float FacingYaw;
    EOKGuardDuty Duty;
    TArray<FOKRTPatrolStop> Route;
};

// A snapshot of live actors, never a second mutable copy of mission progress.
struct FOKRTMissionState
{
    bool bSupplies=false;
    bool bTargetComplete=false;
    int32 PartyCount=0;
    int32 AliveCount=0;
    int32 SafeBankCount=0;
    int32 ExitedCount=0;
};

struct FOKRTMission
{
    int32 Id;
    const TCHAR* Name;
    const TCHAR* Setting;
    const TCHAR* Briefing;
    EOKMissionGoal Goal;
    FVector Spawn;
    FVector Supply;
    FVector Target;
    FVector Exit;
    FVector Hide;
    float MapSize;
    TArray<FOKRTMissionGuard> Guards;
};

namespace OKMissions
{
    constexpr int32 Count=3;
    const FOKRTMission& Get(int32 Id);
    EOKObjectiveStage Stage(const FOKRTMission& Mission,const FOKRTMissionState& State);
    bool CanExecute(const FOKRTMission& Mission,const FOKRTMissionState& State,EOKInteraction Interaction);
    bool CanExtract(const FOKRTMissionState& State);
}
