#pragma once

#include "CoreMinimal.h"

namespace OKDemo
{
    constexpr int32 Width = 11;
    constexpr int32 Height = 9;
    constexpr float CellSize = 180.0f;
    constexpr int32 LevelCount = 3;
    constexpr int32 CabinCells = 4;
    struct FLevel
    {
        const TCHAR* Name;
        FIntPoint Start, TNT, Detonator, Exit;
        int32 BridgeRow;
        int32 GuardPhase;
    };
    OPERACIAKOPANICE_API const FLevel& GetLevel(int32 Index);
    enum class EAction : uint8 { North, East, South, West, Wait, Interact };
    enum class EOutcome : uint8 { Playing, Won, Caught };
    struct FState
    {
        FIntPoint Player = FIntPoint(1, 7);
        int32 LevelIndex = 0;
        int32 Turn = 0;
        bool bTNT = false;
        bool bBridgeDestroyed = false;
        EOutcome Outcome = EOutcome::Playing;
    };
    OPERACIAKOPANICE_API bool IsCabinCell(FIntPoint Cell);
    OPERACIAKOPANICE_API FState InitialState(int32 LevelIndex);
    OPERACIAKOPANICE_API bool IsBlocked(FIntPoint Cell, bool bBridgeDestroyed, int32 LevelIndex = 0);
    OPERACIAKOPANICE_API FIntPoint GuardPosition(int32 Index, int32 Turn, int32 LevelIndex = 0);
    OPERACIAKOPANICE_API FIntPoint GuardFacing(int32 Index, int32 Turn, int32 LevelIndex = 0);
    OPERACIAKOPANICE_API TArray<EAction> FindSolution(int32 LevelIndex);
    OPERACIAKOPANICE_API bool IsThreatened(FIntPoint Cell, const FState& State);
    // Invalid actions never consume a turn. No frame time or random input enters the rules.
    OPERACIAKOPANICE_API bool Apply(FState& State, EAction Action);
}
