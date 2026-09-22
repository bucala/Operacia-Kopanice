#include "ECS/OKTurnCoordinatorSubsystem.h"

bool UOKTurnCoordinatorSubsystem::Transition(EOKTurnPhase Expected, EOKTurnPhase Next)
{
    if (bBroadcasting || Phase != Expected) return false;
    Phase = Next;
    if (Next == EOKTurnPhase::EnemyReaction) ++TurnNumber;
    TGuardValue<bool> Guard(bBroadcasting, true);
    OnPhaseChanged.Broadcast(Phase, TurnNumber);
    return true;
}

bool UOKTurnCoordinatorSubsystem::CommitPlayerAction()
{
    return Transition(EOKTurnPhase::PlayerAction, EOKTurnPhase::EnemyReaction);
}

bool UOKTurnCoordinatorSubsystem::CompleteEnemyReaction()
{
    return Transition(EOKTurnPhase::EnemyReaction, EOKTurnPhase::OutcomeEvaluation);
}

bool UOKTurnCoordinatorSubsystem::CompleteOutcomeEvaluation()
{
    return Transition(EOKTurnPhase::OutcomeEvaluation, EOKTurnPhase::PlayerAction);
}

void UOKTurnCoordinatorSubsystem::ResetTurns()
{
    if (bBroadcasting) return;
    Phase = EOKTurnPhase::PlayerAction;
    TurnNumber = 0;
    TGuardValue<bool> Guard(bBroadcasting, true);
    OnReset.Broadcast();
    OnPhaseChanged.Broadcast(Phase, TurnNumber);
}
