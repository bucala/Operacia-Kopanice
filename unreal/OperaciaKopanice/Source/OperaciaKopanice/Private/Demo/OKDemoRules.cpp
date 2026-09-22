#include "Demo/OKDemoRules.h"

namespace OKDemo
{
    static const FIntPoint Directions[] = {{0,-1}, {1,0}, {0,1}, {-1,0}};
    const FLevel& GetLevel(int32 Index)
    {
        static const FLevel Levels[] = {
            {TEXT("Zelezne hrdlo"), {1,7}, {3,7}, {8,4}, {9,1}, 4, 0},
            {TEXT("Severny brod"), {0,5}, {2,8}, {8,3}, {10,0}, 3, 1},
            {TEXT("Posledny transport"), {0,8}, {5,1}, {7,5}, {10,8}, 5, 2}
        };
        return Levels[FMath::Clamp(Index, 0, LevelCount-1)];
    }
    FState InitialState(int32 LevelIndex)
    {
        FState State;
        State.LevelIndex=FMath::Clamp(LevelIndex,0,LevelCount-1);
        State.Player=GetLevel(State.LevelIndex).Start;
        return State;
    }
    bool IsCabinCell(FIntPoint Cell)
    {
        return Cell.X>=1 && Cell.X<1+CabinCells && Cell.Y>=0 && Cell.Y<CabinCells;
    }
    bool IsBlocked(FIntPoint Cell, bool bBridgeDestroyed, int32 LevelIndex)
    {
        if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= Width || Cell.Y >= Height) return true;
        if (Cell.X == 6 && (Cell.Y != GetLevel(LevelIndex).BridgeRow || bBridgeDestroyed)) return true;
        return IsCabinCell(Cell) ||
            Cell == FIntPoint(4,7) || Cell == FIntPoint(7,1) ||
            Cell == FIntPoint(7,2) || Cell == FIntPoint(9,4);
    }
    FIntPoint GuardPosition(int32 Index, int32 Turn, int32 LevelIndex)
    {
        static const FIntPoint Route[] = {{8,6}, {9,6}, {9,7}, {8,7}};
        return Index == 0 ? FIntPoint(4,4) : Route[(Turn+GetLevel(LevelIndex).GuardPhase) % 4];
    }
    FIntPoint GuardFacing(int32 Index, int32 Turn, int32 LevelIndex)
    {
        return Index == 0 ? Directions[(Turn+GetLevel(LevelIndex).GuardPhase) % 4] :
            GuardPosition(Index, Turn + 1, LevelIndex) - GuardPosition(Index, Turn, LevelIndex);
    }
    bool IsThreatened(FIntPoint Cell, const FState& State)
    {
        for (int32 I = 0; I < 2; ++I)
        {
            FIntPoint Ray = GuardPosition(I, State.Turn, State.LevelIndex);
            if (Ray == Cell) return true;
            const FIntPoint Facing = GuardFacing(I, State.Turn, State.LevelIndex);
            // Two cells leave a timed crossing window at the bridge's eastern end.
            const int32 SightRange = I == 0 ? 2 : 3;
            for (int32 D = 0; D < SightRange; ++D)
            {
                Ray += Facing;
                if (IsBlocked(Ray, State.bBridgeDestroyed, State.LevelIndex)) break;
                if (Ray == Cell) return true;
            }
        }
        return false;
    }
    bool Apply(FState& State, EAction Action)
    {
        if (State.Outcome != EOutcome::Playing) return false;
        FState Next = State;
        const auto& Level=GetLevel(State.LevelIndex);
        const int32 Direction = static_cast<int32>(Action);
        if (Direction < 4)
        {
            Next.Player += Directions[Direction];
            if (IsBlocked(Next.Player, State.bBridgeDestroyed, State.LevelIndex)) return false;
        }
        else if (Action == EAction::Interact)
        {
            if (State.Player == Level.TNT && !State.bTNT) Next.bTNT = true;
            else if (State.Player == Level.Detonator && State.bTNT && !State.bBridgeDestroyed)
                Next.bBridgeDestroyed = true;
            else return false;
        }
        if (IsThreatened(Next.Player, State)) Next.Outcome = EOutcome::Caught;
        ++Next.Turn;
        if (IsThreatened(Next.Player, Next)) Next.Outcome = EOutcome::Caught;
        if (Next.Outcome == EOutcome::Playing && Next.bBridgeDestroyed && Next.Player == Level.Exit)
            Next.Outcome = EOutcome::Won;
        State = Next;
        return true;
    }
    TArray<EAction> FindSolution(int32 LevelIndex)
    {
        struct FNode { FState State; int32 Parent; EAction Action; };
        TArray<FNode> Queue;
        Queue.Add({InitialState(LevelIndex),INDEX_NONE,EAction::Wait});
        TSet<uint32> Visited;
        auto Key=[](const FState& S) {
            return uint32(S.Player.X+Width*S.Player.Y+Width*Height*
                ((S.Turn%4)+4*(int32(S.bTNT)+2*int32(S.bBridgeDestroyed))));
        };
        Visited.Add(Key(Queue[0].State));
        for (int32 Head=0; Head<Queue.Num() && Head<Width*Height*16; ++Head)
        {
            const FState Current=Queue[Head].State;
            if (Current.Outcome==EOutcome::Won)
            {
                TArray<EAction> Route;
                for (int32 I=Head; Queue[I].Parent!=INDEX_NONE; I=Queue[I].Parent)
                    Route.Insert(Queue[I].Action,0);
                return Route;
            }
            for (int32 A=0; A<6; ++A)
            {
                auto Next=Current;
                if (!Apply(Next,EAction(A)) || Next.Outcome==EOutcome::Caught) continue;
                if (Visited.Contains(Key(Next))) continue;
                Visited.Add(Key(Next));
                Queue.Add({Next,Head,EAction(A)});
            }
        }
        return {};
    }
}
