#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTSaveGame.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTPlayerController.h"
#include "Navigation/OKViaductActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    FOKSavedOrder Encode(const FOKRTOrder& O,const TArray<AOKRTUnit*>& Units)
    {
        FOKSavedOrder S; S.Kind=O.Kind; S.Location=O.Location; S.Target=Units.IndexOfByKey(O.Target.Get());
        S.Stance=O.Stance; S.bRun=O.bRunToDestination; S.bApproach=O.bApproachInteraction; S.Interaction=O.Interaction; S.Batch=O.SyncBatch; return S;
    }
    FOKRTOrder Decode(const FOKSavedOrder& S,const TArray<AOKRTUnit*>& Units)
    {
        FOKRTOrder O; O.Kind=S.Kind; O.Location=S.Location; if (Units.IsValidIndex(S.Target)) O.Target=Units[S.Target];
        O.Stance=S.Stance; O.bRunToDestination=S.bRun; O.bApproachInteraction=S.bApproach; O.Interaction=S.Interaction; O.SyncBatch=S.Batch; return O;
    }
    bool Finite(FVector P) { return !P.ContainsNaN() && P.GetAbsMax()<10000000; }
}
FString AOKRTGameMode::SaveSlot() const { return bSmoke ? TEXT("OK_Automation_Quick_v1") : TEXT("OK_Quick_v1"); }
bool AOKRTGameMode::HasQuickSave() const { return UGameplayStatics::DoesSaveGameExist(SaveSlot(),0); }
UOKRTSaveGame* AOKRTGameMode::CaptureSnapshot() const
{
    auto* S=NewObject<UOKRTSaveGame>(); S->MissionId=MissionId; S->SavedAt=FDateTime::UtcNow();
    S->bTNT=bHasTNT; S->bBridge=bBridgeDestroyed; S->bObjective=bObjectiveComplete;
    S->bPaused=bTacticalPause; S->bCommandMode=bCommandMode; S->bPriorPause=bPlanPriorPause;
    S->ActiveMember=ActiveMember; S->CompletedMissions=CompletedMissions;
    S->CameraTarget=CameraTarget; S->CameraYaw=CameraYaw; S->CameraTilt=CameraTilt; S->CameraDistance=CameraDistance;
    S->ActiveBatch=ActiveSyncBatch; S->NextBatch=NextSyncBatch; S->bReleased=bSyncReleased; S->Participants=SyncParticipants;
    S->Stones=Stones; for (auto& Stone:S->Stones) Stone.Visual=nullptr;
    TArray<AOKRTUnit*> Units; for (AOKRTUnit* U:Party) Units.Add(U); for (AOKRTUnit* U:Enemies) Units.Add(U);
    S->DraftValid=DraftValid; for (const auto& O:Drafts) S->Drafts.Add(Encode(O,Units));
    for (auto* U:Units)
    {
        FOKSavedUnit V; V.Feet=U->Feet(); V.Yaw=U->GetActorRotation().Yaw; V.Health=U->Health; V.Cooldown=U->Cooldown;
        V.Distractions=U->Distractions; V.Medkits=U->Medkits; V.Stance=U->Stance; V.bSelected=U->IsSelected();
        V.bHidden=U->bHiddenBody; V.bCone=U->Vision->bConeVisible; V.CarriedBody=Units.IndexOfByKey(U->CarriedBody.Get());
        V.bUnconscious=U->bUnconscious; V.bBound=U->bBound; V.RecoveryRemaining=U->RecoveryRemaining;
        for (const auto& O:U->Orders) V.Orders.Add(Encode(O,Units)); S->Units.Add(V);
    }
    for (AOKRTUnit* U:Enemies)
    {
        auto* AI=CastChecked<AOKRTGuardController>(U->GetController()); FOKSavedGuard V;
        V.Alert=AI->AlertState; V.Suspicion=AI->Suspicion; V.LastKnown=AI->LastKnown; V.PatrolIndex=AI->PatrolIndex;
        V.bAtStop=AI->bAtPatrolStop; V.Wait=AI->Wait; V.LostSight=AI->LostSight; V.ShotClock=AI->ShotClock;
        V.CombatTarget=Party.IndexOfByKey(AI->CombatTarget.Get());
        if (const auto* Body=AI->RescueTarget.Get(); Body && Body->bUnconscious && Body->IsAlive() && AI->AlertState==EOKAlert::Investigate)
            V.RescueTarget=Enemies.IndexOfByKey(Body);
        for (const auto& B:AI->DiscoveredBodies) if (B.IsValid()) V.DiscoveredBodies.Add(Enemies.IndexOfByKey(B.Get()));
        S->Guards.Add(V);
    }
    return S;
}
bool AOKRTGameMode::ValidateSnapshot(const UOKRTSaveGame* S) const
{
    if (!S || S->Version<1 || S->Version>3 || S->MissionId<0 || S->MissionId>=OKMissions::Count) return false;
    if (S->Stones.Num()>16 || (S->Version<3 && !S->Stones.IsEmpty())) return false;
    for (const auto& Stone:S->Stones)
        if (!Finite(Stone.Origin) || !Finite(Stone.Destination) || !FMath::IsFinite(Stone.Elapsed) ||
            !FMath::IsFinite(Stone.Duration) || Stone.Elapsed<0 || Stone.Elapsed>=Stone.Duration ||
            Stone.Duration<.45f || Stone.Duration>1.15f || Stone.PartyMember<0 || Stone.PartyMember>=2 ||
            FVector::Dist2D(Stone.Origin,Stone.Destination)>800 || FMath::Abs(Stone.Origin.Z-Stone.Destination.Z)>400) return false;
    const auto& M=OKMissions::Get(S->MissionId); const int32 Count=2+M.Guards.Num();
    if (S->Units.Num()!=Count || S->Guards.Num()!=M.Guards.Num() || S->ActiveMember<0 || S->ActiveMember>=2 ||
        !Finite(S->CameraTarget) || !FMath::IsFinite(S->CameraYaw) || !FMath::IsFinite(S->CameraTilt) ||
        !FMath::IsFinite(S->CameraDistance) || S->CameraDistance<1000 || S->CameraDistance>20000 ||
        S->CameraTilt<25 || S->CameraTilt>75 || S->CompletedMissions<0 || S->CompletedMissions>=(1<<OKMissions::Count) ||
        (S->bBridge && (M.Goal!=EOKMissionGoal::Bridge || !S->bTNT)) || S->NextBatch<=0 ||
        S->NextBatch>=MAX_int32 || S->ActiveBatch<0 || S->ActiveBatch>=S->NextBatch ||
        (S->ActiveBatch && S->Participants.Num()!=2) || S->Participants.Num()>2 ||
        (!S->ActiveBatch && (!S->Participants.IsEmpty() || S->bReleased)) ||
        S->Drafts.Num()>2 || S->DraftValid.Num()!=S->Drafts.Num() ||
        (!S->bCommandMode && !S->Drafts.IsEmpty()) ||
        (S->bCommandMode && (S->Drafts.Num()!=2 || !S->bPaused || S->ActiveBatch))) return false;
    auto ValidOrder=[Count,S](const FOKSavedOrder& O)
    {
        return uint8(O.Kind)<=uint8(S->Version==1 ? EOKOrder::Heal : S->Version==2 ? EOKOrder::Bind : EOKOrder::Whistle) && uint8(O.Stance)<=uint8(EOKStance::Prone) &&
            uint8(O.Interaction)<=uint8(EOKInteraction::SabotageCommandPost) && Finite(O.Location) &&
            O.Target>=INDEX_NONE && O.Target<Count && (O.Batch==0 || O.Batch==S->ActiveBatch) &&
            ((O.Kind!=EOKOrder::Takedown && O.Kind!=EOKOrder::Knockout && O.Kind!=EOKOrder::Bind && O.Kind!=EOKOrder::Carry) || O.Target==INDEX_NONE || O.Target>=2) &&
            ((O.Kind!=EOKOrder::Takedown && O.Kind!=EOKOrder::Knockout && O.Kind!=EOKOrder::Bind) || O.Target>=2) &&
            (O.Kind!=EOKOrder::Heal || (O.Target>=0 && O.Target<2));
    };
    TSet<int32> Bodies;
    for (int32 I=0;I<S->Units.Num();++I)
    {
        const auto& U=S->Units[I];
        if (!Finite(U.Feet) || !FMath::IsFinite(U.Yaw) || !FMath::IsFinite(U.Health) || U.Health<0 || U.Health>100 ||
            !FMath::IsFinite(U.Cooldown) || U.Cooldown<0 || U.Cooldown>3600 || U.Distractions<0 || U.Distractions>100 ||
            U.Medkits<0 || U.Medkits>100 || uint8(U.Stance)>uint8(EOKStance::Prone) || U.Orders.Num()>32 ||
            !FMath::IsFinite(U.RecoveryRemaining) || U.RecoveryRemaining<0 || U.RecoveryRemaining>3600 ||
            (U.bUnconscious && (I<2 || U.Health<=0)) || (U.bBound && !U.bUnconscious) ||
            ((!U.bUnconscious || U.bBound) && U.RecoveryRemaining!=0) ||
            (S->Version==1 && (U.bUnconscious || U.bBound || U.RecoveryRemaining!=0)) ||
            (U.bHidden && (I<2 || (U.Health>0 && !(U.bUnconscious && U.bBound)))) ||
            U.CarriedBody<INDEX_NONE || U.CarriedBody>=Count) return false;
        if (U.CarriedBody!=INDEX_NONE)
        {
            const auto& B=S->Units[U.CarriedBody];
            if (I>=2 || U.CarriedBody<2 || (B.Health>0 && !(B.bUnconscious && B.bBound)) ||
                B.bHidden || Bodies.Contains(U.CarriedBody) || U.Health<=0 || U.bUnconscious) return false;
            Bodies.Add(U.CarriedBody);
        }
        for (const auto& O:U.Orders)
            if (!ValidOrder(O) || (O.Batch && (I>=2 || !S->Participants[I]))) return false;
    }
    for (const auto& O:S->Drafts) if (!ValidOrder(O)) return false;
    for (int32 I=0;I<S->Guards.Num();++I)
    {
        const auto& G=S->Guards[I];
        if (uint8(G.Alert)>uint8(EOKAlert::Combat) || !FMath::IsFinite(G.Suspicion) || G.Suspicion<0 || G.Suspicion>1 ||
            !Finite(G.LastKnown) || !M.Guards[I].Route.IsValidIndex(G.PatrolIndex) || !FMath::IsFinite(G.Wait) || G.Wait<0 ||
            !FMath::IsFinite(G.LostSight) || G.LostSight<0 || !FMath::IsFinite(G.ShotClock) ||
            G.CombatTarget<INDEX_NONE || G.CombatTarget>=2 || G.DiscoveredBodies.Num()>M.Guards.Num() ||
            G.RescueTarget<INDEX_NONE || G.RescueTarget>=M.Guards.Num() || (S->Version==1 && G.RescueTarget!=INDEX_NONE)) return false;
        if (G.RescueTarget!=INDEX_NONE && (!S->Units[G.RescueTarget+2].bUnconscious ||
            G.RescueTarget==I || G.Alert!=EOKAlert::Investigate || M.Guards[I].Duty!=EOKGuardDuty::Patrol)) return false;
        for (int32 B:G.DiscoveredBodies) if (!S->Guards.IsValidIndex(B)) return false;
    }
    return true;
}
bool AOKRTGameMode::QuickSave()
{
    if (bInitialPausePending || bFrontEnd || bWon || bLost) { Message=TEXT("Teraz nie je mozne ulozit misiu."); return false; }
    auto* Snapshot=CaptureSnapshot();
    if (!ValidateSnapshot(Snapshot) || !UGameplayStatics::SaveGameToSlot(Snapshot,SaveSlot(),0))
    { Message=TEXT("Ulozenie zlyhalo."); return false; }
    Message=TEXT("Misia ulozena."); return true;
}
bool AOKRTGameMode::QuickLoad()
{
    auto* S=Cast<UOKRTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot(),0));
    if (bInitialPausePending || !ValidateSnapshot(S)) { Message=TEXT("Ulozena hra nie je dostupna alebo je nekompatibilna."); return false; }
    // Reassemble the level first: an intact saved bridge cannot be rebuilt from a wreck.
    UGameplayStatics::SetGamePaused(GetWorld(),false);
    UGameplayStatics::OpenLevel(GetWorld(),TEXT("/Engine/Maps/Entry"),true,FString::Printf(TEXT("Mission=%d?Deploy?QuickLoad"),S->MissionId));
    return true;
}
bool AOKRTGameMode::ApplySnapshot(const UOKRTSaveGame* S)
{
    if (!ValidateSnapshot(S) || S->MissionId!=MissionId || (bBridgeDestroyed && !S->bBridge)) return false;
    UGameplayStatics::SetGamePaused(GetWorld(),true);
    for (auto& Stone:Stones) if (Stone.Visual) Stone.Visual->Destroy();
    Stones=S->Stones; for (auto& Stone:Stones) { Stone.Visual=nullptr; CreateStoneVisual(Stone); }
    ActiveSyncBatch=0; bCommandMode=false; bWon=bLost=false; bMenu=bOptions=bFrontEnd=false;
    TArray<AOKRTUnit*> Units; for (AOKRTUnit* U:Party) Units.Add(U); for (AOKRTUnit* U:Enemies) Units.Add(U);
    for (int32 I=0;I<Units.Num();++I)
    {
        auto* U=Units[I]; const auto& V=S->Units[I];
        U->CancelOrders(); U->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        U->CarriedBody.Reset(); U->Carrier.Reset(); U->Health=100; U->bUnconscious=U->bBound=false; U->RecoveryRemaining=0; U->bHiddenBody=false; U->SetActorHiddenInGame(false);
        U->Visual->SetRelativeRotation(FRotator::ZeroRotator);
        U->Visual->SetRelativeLocation(FVector(0,0,-87));
        U->SmoothedSpeed=U->MotionBlend=0;
        U->CrouchBlend=V.Stance==EOKStance::Crouch ? 1.f : 0.f;
        U->ProneBlend=V.Stance==EOKStance::Prone ? 1.f : 0.f;
        U->GetCharacterMovement()->UnCrouch(false); U->GetCapsuleComponent()->SetCapsuleSize(30,90);
        U->GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn")); U->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        U->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        U->GetCharacterMovement()->SetMovementMode(MOVE_Walking); U->GetCharacterMovement()->StopMovementImmediately();
        U->SetStance(V.Stance); U->SetActorLocation(V.Feet+FVector(0,0,90),false,nullptr,ETeleportType::TeleportPhysics);
        U->SetActorRotation(FRotator(0,V.Yaw,0));
        if (V.Health<=0) U->TakeHit(100); else { U->Health=V.Health; U->UpdateAnimation(0); }
        if (V.bUnconscious)
        { U->bUnconscious=true; U->bBound=V.bBound; U->RecoveryRemaining=V.RecoveryRemaining; U->IncapacitatedPose(); }
        U->bHiddenBody=V.bHidden; U->SetActorHiddenInGame(V.bHidden);
        if (V.bHidden) U->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (V.bHidden) U->Stimuli->UnregisterFromPerceptionSystem(); else U->Stimuli->RegisterWithPerceptionSystem();
        U->Select(V.bSelected); U->Distractions=V.Distractions; U->Medkits=V.Medkits; U->Cooldown=V.Cooldown;
        U->Vision->bConeVisible=V.bCone; U->NoiseClock=0; U->ReadyBatch=0;
    }
    for (int32 I=0;I<Units.Num();++I)
    {
        auto* U=Units[I]; const auto& V=S->Units[I];
        if (Units.IsValidIndex(V.CarriedBody))
        {
            auto* Body=Units[V.CarriedBody]; U->CarriedBody=Body; Body->Carrier=U;
            Body->GetCapsuleComponent()->SetCollisionProfileName(TEXT("NoCollision"));
            Body->AttachToActor(U,FAttachmentTransformRules::KeepWorldTransform); Body->SetActorRelativeLocation(FVector(-35,0,50)); U->SetStance(U->Stance);
        }
        for (const auto& O:V.Orders) U->Orders.Add(Decode(O,Units));
    }
    for (int32 I=0;I<Enemies.Num();++I)
    {
        auto* AI=CastChecked<AOKRTGuardController>(Enemies[I]->GetController()); const auto& V=S->Guards[I];
        AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay); AI->PursuitTarget.Reset();
        AI->AlertState=V.Alert; AI->Suspicion=V.Suspicion; AI->LastKnown=V.LastKnown;
        AI->PatrolIndex=V.PatrolIndex; AI->bAtPatrolStop=V.bAtStop; AI->Wait=V.Wait; AI->LostSight=V.LostSight; AI->ShotClock=V.ShotClock; AI->MoveRetry=0;
        AI->CombatTarget=Party.IsValidIndex(V.CombatTarget) ? Party[V.CombatTarget].Get() : nullptr;
        AI->RescueTarget=Enemies.IsValidIndex(V.RescueTarget) ? Enemies[V.RescueTarget].Get() : nullptr;
        AI->DiscoveredBodies.Reset(); for (int32 B:V.DiscoveredBodies) AI->DiscoveredBodies.Add(Enemies[B].Get());
    }
    bHasTNT=S->bTNT; bObjectiveComplete=S->bObjective; if (S->bBridge && !bBridgeDestroyed) BreakBridge();
    if (TNTMarker) TNTMarker->SetActorHiddenInGame(bHasTNT || bObjectiveComplete);
    ActiveMember=S->ActiveMember; CompletedMissions|=S->CompletedMissions;
    CameraTarget=S->CameraTarget; CameraYaw=S->CameraYaw; CameraTilt=S->CameraTilt; CameraDistance=S->CameraDistance; UpdateCamera();
    bCommandMode=S->bCommandMode; bPlanPriorPause=S->bPriorPause; DraftValid=S->DraftValid;
    Drafts.Reset(); for (const auto& O:S->Drafts) Drafts.Add(Decode(O,Units));
    ActiveSyncBatch=S->ActiveBatch; NextSyncBatch=S->NextBatch; bSyncReleased=S->bReleased; SyncParticipants=S->Participants;
    NoisePulses.Reset(); PreviewSpline->ClearSplinePoints(); CommandFeedback={};
    if (auto* PC=Cast<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController())) { PC->CancelPointer(); PC->bArmed=false; }
    bTacticalPause=S->bPaused; UGameplayStatics::SetGamePaused(GetWorld(),bTacticalPause);
    Message=TEXT("Misia obnovena."); return true;
}
void AOKRTGameMode::BreakBridge()
{
    if (!Bridge || bBridgeDestroyed) return;
    bBridgeDestroyed=true; Bridge->DestroySpan();
    TArray<UStaticMeshComponent*> Parts; Bridge->GetComponents(Parts);
    for (auto* Part:Parts) { Part->SetCollisionProfileName(TEXT("NoCollision")); Part->SetCanEverAffectNavigation(false); }
    BridgeFloor->SetCollisionProfileName(TEXT("NoCollision")); BridgeFloor->SetCanEverAffectNavigation(false);
    FNavigationSystem::UpdateComponentData(*BridgeFloor);
}
