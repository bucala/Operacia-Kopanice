#pragma once
#include "CoreMinimal.h"

enum class EOKMissionGoal : uint8 { Bridge, Documents, CommandPost };
enum class EOKMenuPage : uint8 { Pause, Missions, Briefing, Options, ConfirmRestart, ConfirmQuit };

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
};

namespace OKMissions
{
    constexpr int32 Count=3;
    const FOKRTMission& Get(int32 Id);
}
