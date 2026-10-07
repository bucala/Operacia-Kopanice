#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTHUD.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "Navigation/PathFollowingComponent.h"

void AOKRTGameMode::Require(bool Condition,const TCHAR* Label)
{
    if (Condition) { UE_LOG(LogTemp,Display,TEXT("OK_RT_CHECK: %s PASS"),Label); }
    else { UE_LOG(LogTemp,Error,TEXT("OK_RT_CHECK: %s FAIL"),Label); }
    if (!Condition)
    {
        SmokeStage=-1;
        FPlatformMisc::RequestExitWithStatus(false,1);
    }
}
void AOKRTGameMode::SmokeTick()
{
    const double Now=FPlatformTime::Seconds();
    if (SmokeStage<0) return;
    static double LastDiagnostic=0;
    static bool bObservedPatrolMove=false;
    if (Now-LastDiagnostic>5)
    {
        LastDiagnostic=Now;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_PROGRESS: stage=%d feet=%s velocity=%s queue=%d message=%s"),
            SmokeStage,*Party[0]->Feet().ToString(),*Party[0]->GetVelocity().ToString(),Party[0]->QueueSize(),*Message);
    }
    if (Now-SmokeStarted>170) { Require(false,TEXT("runtime timeout")); return; }
    auto Next=[this,Now]() { if (SmokeStage<0) return; ++SmokeStage; StageStarted=Now; UE_LOG(LogTemp,Display,TEXT("OK_RT_STAGE: %d"),SmokeStage); };
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto Key=[PC](FKey K)
    { PC->InputKey(FInputKeyParams(K,IE_Pressed,1.0)); PC->InputKey(FInputKeyParams(K,IE_Released,0.0)); };
    auto Move=[](AOKRTUnit* Unit,FVector Location,bool Append=false) { FOKRTOrder O; O.Location=Location; Unit->Submit(O,Append); };
    auto Order=[](AOKRTUnit* Unit,EOKOrder Kind,AOKRTUnit* Target=nullptr)
    { FOKRTOrder O; O.Kind=Kind; O.Target=Target; Unit->Submit(O,true); };
    auto* GuardAI=CastChecked<AOKRTGuardController>(Enemies[1]->GetController());
    switch (SmokeStage)
    {
    case 0:
        if (Now-StageStarted<4) return;
        if (!HasCompletePath(Party[0]->Feet(),FVector(1440,720,0)))
        {
            if (Now-StageStarted>25) Require(false,TEXT("NavMesh generated and bridge connects banks"));
            return;
        }
        Require(true,TEXT("NavMesh generated and bridge connects banks"));
        Require(bTacticalPause && UGameplayStatics::IsGamePaused(GetWorld()),TEXT("initial planning pause waits for NavMesh"));
        TogglePause();
        Require(CastChecked<AOKRTHUD>(PC->GetHUD())->ValidateLayout(),TEXT("HUD button bounds and separation"));
        Require(Camera && Camera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount &&
            Camera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount==0.f &&
            Camera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurMax &&
            Camera->GetCameraComponent()->PostProcessSettings.MotionBlurMax==0.f,
            TEXT("zoom keeps paused camera crisp without motion blur"));
        Require(!HasCompletePath(Party[0]->Feet(),FVector(1080,1260,0)),TEXT("river is not walkable"));
        for (AOKRTUnit* Unit:Party) Require(Unit->Visual->GetStaticMesh()!=nullptr,TEXT("party imported mesh"));
        for (AOKRTUnit* Unit:Enemies) Require(Unit->Visual->GetStaticMesh()!=nullptr,TEXT("guard imported mesh"));
        for (AOKRTUnit* Unit : {Party[0].Get(),Party[1].Get(),Enemies[0].Get(),Enemies[1].Get()})
        {
            Require(Unit->AnimatedVisual->GetSkinnedAsset()!=nullptr &&
                Unit->AnimatedVisual->GetBoneIndex(TEXT("thigh_l"))!=INDEX_NONE,
                TEXT("party and guards use imported skinned rigs"));
            Require(FMath::IsNearlyEqual(Unit->GetVisualHeight(),180.f,.5f),TEXT("character height is 180 cm"));
        }
        {
            auto* Unit=Party[0].Get();
            const FVector Velocity=Unit->GetCharacterMovement()->Velocity;
            const FTransform Before=Unit->AnimatedVisual->GetBoneTransformByName(TEXT("thigh_l"),EBoneSpaces::ComponentSpace);
            const FVector PelvisBefore=Unit->AnimatedVisual->GetBoneLocationByName(TEXT("pelvis"),EBoneSpaces::ComponentSpace);
            Unit->GetCharacterMovement()->Velocity=FVector(190,0,0);
            for (int32 Frame=0;Frame<15;++Frame) Unit->Tick(1.f/60);
            const FTransform After=Unit->AnimatedVisual->GetBoneTransformByName(TEXT("thigh_l"),EBoneSpaces::ComponentSpace);
            Require(!Before.GetRotation().Equals(After.GetRotation(),.01f),TEXT("real-time velocity animates leg bones"));
            float MaxSway=0,MaxBob=0;
            for (int32 Frame=0;Frame<120;++Frame)
            {
                Unit->Tick(1.f/60);
                const FVector Offset=Unit->AnimatedVisual->GetBoneLocationByName(TEXT("pelvis"),EBoneSpaces::ComponentSpace)-PelvisBefore;
                MaxSway=FMath::Max(MaxSway,float(Offset.Size2D()));
                MaxBob=FMath::Max(MaxBob,float(FMath::Abs(Offset.Z)));
            }
            Require(MaxSway<.01f && MaxBob<=.61f,TEXT("gait pelvis has no lateral sway and sub-centimetre bob"));
            UE_LOG(LogTemp,Display,TEXT("OK_RT_GAIT: lateral=%.4f cm vertical=%.4f cm"),MaxSway,MaxBob);
            Unit->GetCharacterMovement()->Velocity=Velocity;
        }
        {
            auto* Unit=Party[0].Get();
            auto* AI=CastChecked<AOKRTGuardController>(Unit->GetController());
            FOKRTOrder MoveOrder; MoveOrder.Location=FVector(180,960,0);
            Unit->Submit(MoveOrder);
            Unit->Tick(0);
            const auto Request=AI->GetPathFollowingComponent()->GetCurrentRequestId();
            FOKRTOrder Attack; Attack.Kind=EOKOrder::Takedown; Attack.Target=Party[1];
            Require(!Unit->Submit(Attack) && Unit->QueueSize()==1 &&
                AI->GetPathFollowingComponent()->GetCurrentRequestId()==Request,
                TEXT("friendly takedown rejected without cancelling movement"));
            FOKRTOrder Carry; Carry.Kind=EOKOrder::Carry; Carry.Target=Enemies[0];
            Require(!Unit->Submit(Carry) && Unit->QueueSize()==1,TEXT("living guard cannot be carried"));
            Unit->CancelOrders();
            Attack.Target=Enemies[0];
            Require(Unit->Submit(Attack,true) && Unit->Submit(Carry,true) && Unit->QueueSize()==2,
                TEXT("takedown then carry can be planned together"));
            Unit->Tick(0);
            Enemies[0]->Health=0;
            Unit->Tick(0);
            Require(Unit->QueueSize()==1 && AI->GetMoveStatus()==EPathFollowingStatus::Idle,
                TEXT("stale takedown stops its active path"));
            Unit->CancelOrders(); Enemies[0]->Health=100;
            FOKRTOrder Throw; Throw.Kind=EOKOrder::Distract; Throw.Location=Unit->Feet()+FVector(0,100,0);
            const int32 Ammo=Unit->Distractions;
            Unit->Cooldown=1; Unit->Submit(Throw); Unit->Tick(0);
            Require(Unit->QueueSize()==1 && Unit->Distractions==Ammo,TEXT("cooldown retains queued distraction"));
            Unit->Cooldown=0; Unit->Tick(0);
            Require(Unit->QueueSize()==0 && Unit->Distractions==Ammo-1 && Unit->Cooldown==6,
                TEXT("ready distraction executes exactly once"));
            Unit->Distractions=0;
            Require(!Unit->Submit(Throw) && Unit->QueueSize()==0,TEXT("empty inventory rejects distraction"));
            Unit->Distractions=Ammo; Unit->Cooldown=0;
            Select(0); SelectAll();
            Require(Command(Throw) && Unit->QueueSize()==1 && Party[1]->QueueSize()==0,
                TEXT("group ability only queues on active member"));
            Unit->CancelOrders(); Select(0);
            for (int32 I=0;I<32;++I) Unit->Submit(MoveOrder,true);
            Require(!Unit->Submit(MoveOrder,true) && Unit->QueueSize()==32,TEXT("queue limit reports rejection"));
            Unit->CancelOrders();
            ToggleMenu(); PC->Arm(EOKOrder::Takedown);
            Require(!PC->bArmed,TEXT("menu prevents arming gameplay actions"));
            ToggleMenu();
        }
        {
            const FVector Saved=Party[1]->GetActorLocation();
            const FTransform GuardPose=Enemies[0]->GetActorTransform();
            Enemies[0]->SetActorLocation(FVector(720,720,90));
            Enemies[0]->SetActorRotation(FRotator(0,90,0));
            Party[1]->SetActorLocation(FVector(720,930,90));
            bool Primary=false;
            Require(Enemies[0]->Vision->Sees(Party[1],Primary) && Primary,TEXT("primary cone geometry"));
            Party[1]->SetActorLocation(FVector(450,270,90));
            Enemies[0]->SetActorRotation((Party[1]->Feet()-Enemies[0]->Feet()).Rotation());
            Require(!Enemies[0]->Vision->Sees(Party[1],Primary),TEXT("cabin occludes sight"));
            Enemies[0]->SetActorRotation(FRotator(0,90,0));
            Party[1]->SetActorLocation(Saved);
            Party[0]->SetStance(EOKStance::Crouch);
            Require(Party[0]->VisibilityFactor()<.4f,TEXT("crouch lowers detection"));
            Party[0]->SetStance(EOKStance::Walk);
            const FVector SavedFirst=Party[0]->GetActorLocation();
            Party[0]->SetActorLocation(FVector(720,1320,90));
            Party[1]->SetActorLocation(FVector(180,720,90));
            auto* AI=CastChecked<AOKRTGuardController>(Enemies[0]->GetController());
            AI->bBrainEnabled=true; AI->Suspicion=0;
            Party[0]->SetStance(EOKStance::Crouch); AI->Tick(.25f);
            const float Crouched=AI->Suspicion;
            AI->Suspicion=0; Party[0]->SetStance(EOKStance::Walk); AI->Tick(.25f);
            Require(Crouched>.02f && Crouched<.05f && AI->Suspicion>.08f && AI->Suspicion<.13f,
                TEXT("far-zone continuous suspicion scales with stance"));
            AI->bBrainEnabled=false; AI->Suspicion=0; AI->StopMovement();
            Party[0]->SetActorLocation(SavedFirst); Party[1]->SetActorLocation(Saved);
            Enemies[0]->SetActorTransform(GuardPose);
        }
        {
            auto* AI=CastChecked<AOKRTGuardController>(Enemies[0]->GetController());
            const FVector GuardLocation=Enemies[0]->GetActorLocation();
            const FRotator GuardRotation=Enemies[0]->GetActorRotation();
            const FVector PartyLocation=Party[0]->GetActorLocation();
            Enemies[0]->SetActorLocation(FVector(900,450,90));
            Enemies[0]->SetActorRotation(FRotator(0,90,0));
            Party[0]->SetActorLocation(FVector(900,1260,90));
            AI->bBrainEnabled=true; AI->Alert(Party[0],false); AI->Tick(.016f);
            auto* Path=AI->GetPathFollowingComponent();
            const auto Request=Path->GetCurrentRequestId();
            AI->Tick(.016f);
            Require(Path->GetMoveGoal()==Party[0] && Path->GetCurrentRequestId()==Request,
                TEXT("combat pursuit reuses moving actor path"));
            const FVector LastSeen=AI->LastKnown;
            Enemies[0]->Vision->FarRange=0; Enemies[0]->Vision->NearRange=0;
            Party[0]->SetActorLocation(FVector(180,960,90)); AI->Tick(.016f);
            Require(Path->GetMoveGoal()==nullptr && AI->LastKnown.Equals(LastSeen,.1),
                TEXT("lost sight follows last seen position not hidden actor"));
            AI->Tick(6.1f);
            Require(AI->AlertState==EOKAlert::Investigate,TEXT("lost target transitions from combat to search"));
            AI->bBrainEnabled=false; AI->StopMovement(); AI->AlertState=EOKAlert::Patrol; AI->Suspicion=0;
            Enemies[0]->Vision->FarRange=1000; Enemies[0]->Vision->NearRange=330;
            Enemies[0]->SetActorLocation(GuardLocation); Enemies[0]->SetActorRotation(GuardRotation);
            Party[0]->SetActorLocation(PartyLocation);
        }
        SmokeNavigationTests();
        if (SmokeStage<0) return;
        SmokeControlsTests();
        if (SmokeStage<0) return;
        Preview(FVector(720,1260,0));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_Start.png"),false,false);
        bObservedPatrolMove=false; Enemies[1]->Vision->FarRange=0;
        // Short route for the isolated locomotion test; mission timing has its own live-AI test.
        GuardAI->PatrolRoute={{Enemies[1]->Feet(),90,0}, {FVector(1740,1400,0),90,1}};
        Move(Party[0],FVector(720,1260,0));
        Next(); break;
    case 1:
        // Let hearing events from the isolated inventory checks drain before patrol.
        if (Now-StageStarted>.5) GuardAI->bBrainEnabled=true;
        bObservedPatrolMove|=FVector::Dist2D(Enemies[1]->Feet(),FVector(1740,1200,0))>20;
        if (FVector::Dist2D(Party[0]->Feet(),FVector(720,1260,0))>65 || Party[0]->QueueSize()) return;
        Require(true,TEXT("continuous AI MoveTo arrival"));
        Require(GuardAI->AlertState==EOKAlert::Patrol && bObservedPatrolMove,
            TEXT("real-time guard patrol movement"));
        GuardAI->bBrainEnabled=false; GuardAI->StopMovement(); Enemies[1]->Vision->FarRange=1000;
        PausePosition=Party[0]->GetActorLocation();
        Key(EKeys::SpaceBar);
        Next(); break;
    case 2:
        if (Now-StageStarted<1.5) return;
        Require(UGameplayStatics::IsGamePaused(GetWorld()),TEXT("actual world pause"));
        Require(FVector::Dist(PausePosition,Party[0]->GetActorLocation())<.1,TEXT("paused movement frozen"));
        Move(Party[0],FVector(810,1260,0));
        { FOKRTOrder O; O.Kind=EOKOrder::Stance; O.Stance=EOKStance::Crouch; Party[0]->Submit(O,true); }
        Require(Party[0]->QueueSize()==2,TEXT("paused command queue"));
        {
            TArray<FVector> QueuedLocations; Party[0]->GetQueuedLocations(QueuedLocations);
            Require(QueuedLocations.Num()==1 && QueuedLocations[0].Equals(FVector(810,1260,0),65.f),
                TEXT("queued NavMesh route is exposed to tactical HUD"));
        }
        SmokeSelectionTests();
        if (SmokeStage<0) return;
        SmokeMarkerTests();
        if (SmokeStage<0) return;
        Orbit(45,0); Orbit(-45,0);
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_Pause.png"),false,false);
        Next(); break;
    case 3:
        // Leave pause visible for a rendered frame before resuming the queued mission.
        if (bTacticalPause)
        {
            if (Now-StageStarted<.5) return;
            Select(0); Key(EKeys::SpaceBar); return;
        }
        if (Party[0]->QueueSize()) return;
        Require(Party[0]->Stance==EOKStance::Crouch && FVector::Dist2D(Party[0]->Feet(),FVector(810,1260,0))<65,
            TEXT("resume executes queued movement and stance"));
        Enemies[0]->SetActorLocation(FVector(900,1260,90)); Enemies[0]->SetActorRotation(FRotator::ZeroRotator);
        Party[0]->Cooldown=.6f;
        Order(Party[0],EOKOrder::Takedown,Enemies[0]); Party[0]->Tick(0);
        Require(Party[0]->QueueSize()==1 && Enemies[0]->IsAlive(),TEXT("cooldown retains queued takedown"));
        Next(); break;
    case 4:
        if (Party[0]->QueueSize()) return;
        Require(!Enemies[0]->IsAlive(),TEXT("rear stealth takedown"));
        Order(Party[0],EOKOrder::Carry,Enemies[0]); Next(); break;
    case 5:
        if (Party[0]->QueueSize()) return;
        Require(Party[0]->CarriedBody==Enemies[0],TEXT("body carrying"));
        { FOKRTOrder O; O.Kind=EOKOrder::Interact; O.Location=HideLocation; O.bApproachInteraction=true;
          O.Interaction=EOKInteraction::HideBody; Party[0]->Submit(O); }
        Next(); break;
    case 6:
        if (Party[0]->QueueSize()) return;
        Require(Enemies[0]->bHiddenBody && !Party[0]->CarriedBody.IsValid(),TEXT("body hidden in cover"));
        GuardAI->bBrainEnabled=true;
        Enemies[1]->Vision->FarRange=0;
        { FOKRTOrder O; O.Kind=EOKOrder::Distract; O.Location=FVector(900,1260,0); Party[1]->Submit(O); }
        Next(); break;
    case 7:
        if (Now-StageStarted<2) return;
        Require(Party[1]->Distractions==4 && Party[1]->Cooldown>0,TEXT("distraction ammo and cooldown"));
        Require(GuardAI->AlertState==EOKAlert::Investigate,TEXT("AI hearing investigates distraction"));
        Require(FVector::Dist2D(Enemies[1]->Feet(),FVector(1740,1200,0))>20,TEXT("guard investigation uses continuous movement"));
        GuardAI->bBrainEnabled=false; GuardAI->StopMovement();
        { FOKRTOrder O; O.Kind=EOKOrder::Interact; O.Location=TNTLocation; O.bApproachInteraction=true;
          O.Interaction=EOKInteraction::CollectTNT; Party[0]->Submit(O); }
        Move(Party[0],DetonatorLocation+FVector(0,-45,0),true);
        Move(Party[1],DetonatorLocation+FVector(0,45,0));
        Next(); break;
    case 8:
        if (Party[0]->QueueSize() || Party[1]->QueueSize()) return;
        Require(bHasTNT,TEXT("TNT pickup"));
        Require(Party[0]->Feet().X>1195 && Party[1]->Feet().X>1195,TEXT("both party members cross actual bridge path"));
        {
            const FTransform Saved=Party[1]->GetActorTransform();
            const int32 Pulses=NoisePulses.Num();
            Party[1]->SetActorLocation(FVector(720,1260,Saved.GetLocation().Z));
            Interact(Party[0]);
            Require(!bBridgeDestroyed && NoisePulses.Num()==Pulses &&
                HasCompletePath(FVector(720,720,0),DetonatorLocation),
                TEXT("sabotage cannot strand a teammate on the west bank or emit an explosion"));
            Require(Objective()==TEXT("Prejdite most: 1/2 v bezpeci"),TEXT("objective reports partial party crossing"));
            const float Radius=Party[1]->GetCapsuleComponent()->GetScaledCapsuleRadius();
            Party[1]->SetActorLocation(FVector(BridgeFloor->Bounds.GetBox().Max.X+Radius-1,720,Saved.GetLocation().Z));
            Interact(Party[0]);
            Require(!bBridgeDestroyed && NoisePulses.Num()==Pulses,
                TEXT("sabotage waits until the entire teammate capsule clears the span"));
            Party[1]->SetActorTransform(Saved);
            Require(Objective()==TEXT("Aktivujte detonator"),TEXT("detonator objective unlocks when both members are safe"));
        }
        Order(Party[0],EOKOrder::Interact); Next(); break;
    case 9:
        if (Now-StageStarted<3) return;
        Require(bBridgeDestroyed,TEXT("sabotage objective"));
        Require(!HasCompletePath(FVector(720,720,0),FVector(1440,720,0)),TEXT("destroyed bridge invalidates NavMesh"));
        {
            FOKRTOrder Exit; Exit.Location=ExitLocation;
            Require(Party[0]->Submit(Exit),TEXT("east-bank route remains available after sabotage"));
            Party[0]->Tick(0);
            const auto* AI=CastChecked<AOKRTGuardController>(Party[0]->GetController());
            const auto Request=AI->GetPathFollowingComponent()->GetCurrentRequestId();
            FOKRTOrder Across; Across.Location=FVector(720,720,0);
            Require(!Party[0]->Submit(Across) && Party[0]->QueueSize()==1 &&
                AI->GetPathFollowingComponent()->GetCurrentRequestId()==Request,
                TEXT("destroyed bridge crossing is rejected without cancelling extraction"));
            Party[0]->CancelOrders();
        }
        Move(Party[0],ExitLocation+FVector(0,-40,0)); Move(Party[1],ExitLocation+FVector(0,40,0));
        Next(); break;
    case 10:
        if (!bWon) return;
        Require(!bLost,TEXT("real-time extraction victory"));
        Require(GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>()->TurnNumber==0,TEXT("no turn coordinator dependency"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_Win.png"),false,false); Next(); break;
    case 11:
        if (Now-StageStarted<2) return;
        bWon=false; bTacticalPause=false; UGameplayStatics::SetGamePaused(GetWorld(),false);
        for (AOKRTUnit* U:Party) U->CancelOrders();
        Party[0]->SetActorLocation(FVector(1360,450,90)); Party[0]->SetStance(EOKStance::Walk);
        Party[1]->SetActorLocation(FVector(1750,180,90));
        Enemies[1]->SetActorLocation(FVector(1620,430,90)); Enemies[1]->SetActorRotation(FRotator(0,180,0));
        bBridgeDestroyed=false; // Disable extraction only for the separate combat/loss fixture.
        GuardAI->AlertState=EOKAlert::Patrol; GuardAI->Suspicion=0; GuardAI->bBrainEnabled=true;
        Enemies[1]->Vision->FarRange=1000;
        Next(); break;
    case 12:
        if (Now-StageStarted<1) return;
        Require(GuardAI->AlertState==EOKAlert::Combat,TEXT("primary-zone standing detection triggers combat"));
        Next(); break;
    case 13:
        if (!bLost) return;
        Require(!Party[0]->IsAlive() || !Party[1]->IsAlive(),TEXT("combat damage and loss outcome"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_Loss.png"),false,false);
        Next(); break;
    case 14:
        if (Now-StageStarted<2) return;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_SMOKE: PASS"));
        FPlatformMisc::RequestExit(false); bSmoke=false; break;
    }
}
