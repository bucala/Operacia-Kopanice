#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTSaveGame.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTHUD.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UnrealClient.h"
#include <limits>

void AOKRTGameMode::DistractionSmokeTick()
{
    if (SmokeStage<0 || bInitialPausePending) return;
    const double Now=FPlatformTime::Seconds();
    if (Now-SmokeStarted>80) { Require(false,TEXT("distraction fixture timeout")); return; }
    auto Next=[&]() { if (SmokeStage>=0) { ++SmokeStage; StageStarted=Now; } };
    auto* P=Party[0].Get(); auto* O=Party[1].Get(); auto* Guard=Enemies[0].Get();
    auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController());
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* HUD=CastChecked<AOKRTHUD>(PC->GetHUD());
    auto Place=[](AOKRTUnit* U,FVector Feet,float Yaw=0)
    {
        U->CancelOrders(); U->SetStance(EOKStance::Walk);
        U->SetActorLocation(Feet+FVector(0,0,90),false,nullptr,ETeleportType::TeleportPhysics);
        U->SetActorRotation(FRotator(0,Yaw,0)); U->GetCharacterMovement()->StopMovementImmediately();
    };
    auto Key=[PC,this](FKey K)
    {
        for (EInputEvent Event:{IE_Pressed,IE_Released})
        {
            PC->InputKey(FInputKeyParams(K,Event,Event==IE_Pressed ? 1. : 0.));
            PC->PlayerInput->ProcessInputStack({PC->InputComponent},0,GetWorld()->IsPaused());
        }
    };
    FOKRTOrder Throw; Throw.Kind=EOKOrder::Distract; Throw.Location=FVector(700,1100,-8);
    FOKRTOrder Whistling; Whistling.Kind=EOKOrder::Whistle;
    switch (SmokeStage)
    {
    case 0:
    {
        Require(bTacticalPause && GetWorld()->IsPaused(),TEXT("distraction fixture explicitly pauses simulation"));
        Place(P,FVector(400,1100,-8)); Place(O,FVector(200,1300,-8));
        Place(Guard,FVector(880,1100,-8)); Place(Enemies[1],FVector(1700,1400,-8)); Select(0);
        TArray<FVector> Points;
        Require(StoneTrajectory(P,P->Feet(),Throw.Location,Points) && Points.Num()==25 && Points[12].Z>Points[0].Z+70,
            TEXT("stone follows a sampled raised arc to supported terrain"));
        Require(PreviewOrder(Throw) && PreviewSpline->GetNumberOfSplinePoints()==25 && P->Distractions==5 && Stones.IsEmpty(),
            TEXT("hover preview draws the real arc without spending items or launching"));
        auto* Low=Collider(FVector(550,1100,40),FVector(8,70,45),TEXT("StoneLowWall"));
        Require(StoneTrajectory(P,P->Feet(),Throw.Location,Points),TEXT("raised arc clears low cover")); Low->GetOwner()->Destroy();
        auto* Wall=Collider(FVector(550,1100,200),FVector(8,100,220),TEXT("StoneHighWall"));
        Require(!P->Submit(Throw) && P->QueueSize()==0 && P->Distractions==5,TEXT("high wall rejects throw before queue or inventory mutation"));
        Require(!NoiseAudible(Guard,P,P->Feet(),700) && NoiseAudible(Guard,P,P->Feet(),1800),
            TEXT("walls reduce hearing to thirty percent of event range"));
        Wall->GetOwner()->Destroy();
        Require(NoiseAudible(Guard,P,P->Feet(),700) && !NoiseAudible(Enemies[1],P,P->Feet(),700),
            TEXT("unobstructed hearing respects finite local radius"));
        FOKRTOrder Far=Throw; Far.Location=FVector(2000,1100,-8);
        Require(!P->Submit(Far),TEXT("out of range stone cannot launch"));
        Far.Location=FVector(700,1100,190); Require(!P->Submit(Far),TEXT("unsupported midair destination rejected"));
        FOKRTOrder Move; Move.Location=FVector(600,1100,-8);
        Far.Location=FVector(1300,1100,-8);
        Require(P->Submit(Move) && P->Submit(Far,true) && P->GetOrderPath(Far,true,Points,Message) &&
            FMath::IsNearlyEqual(Points[0].X,600.),TEXT("queued throw predicts from preceding move rather than current feet"));
        P->CancelOrders(); P->CarriedBody=Enemies[1];
        Require(!P->Submit(Throw) && !P->Submit(Whistling),TEXT("both distraction skills require free hands")); P->CarriedBody.Reset();
        Key(EKeys::Y); FOKRTOrder Queued;
        Require(P->GetQueuedOrder(0,Queued) && Queued.Kind==EOKOrder::Whistle && NoisePulses.IsEmpty(),
            TEXT("Y queues a whistle during true pause without emitting sound"));
        P->CancelOrders(); Select(1); Key(EKeys::Y);
        Require(O->QueueSize()==0 && NoisePulses.IsEmpty(),TEXT("officer cannot use partisan whistle")); Select(0);
        Require(P->Submit(Throw),TEXT("stone order accepted")); P->Tick(0);
        Require(Stones.IsEmpty() && P->Distractions==5,TEXT("scheduled paused unit tick cannot launch stone"));
        TogglePause(); P->UpdateOrders(); TogglePause();
        Require(Stones.Num()==1 && Stones[0].Visual && P->Distractions==4 && P->Cooldown==6 && NoisePulses.IsEmpty(),
            TEXT("execution spends one stone but emits no impact noise at launch"));
        TogglePause(); UpdateStones(.2f); TogglePause();
        Require(Stones.Num()==1 && Stones[0].Elapsed>0 && NoisePulses.IsEmpty(),TEXT("airborne stone advances before impact without premature sound"));
        const float Time=Stones[0].Elapsed; UpdateStones(2);
        Require(Stones.Num()==1 && Stones[0].Elapsed==Time && NoisePulses.IsEmpty(),TEXT("pause freezes projectile and sound"));
        auto* S=CaptureSnapshot(); Require(S->Version==3 && ValidateSnapshot(S) && S->Stones.Num()==1,TEXT("schema 3 captures airborne stone"));
        S->Stones[0].Elapsed=std::numeric_limits<float>::quiet_NaN();
        Require(!ApplySnapshot(S) && Stones.Num()==1,TEXT("corrupt flight rejected before mutating live world"));
        S->Stones[0].Elapsed=Time; S->Stones[0].PartyMember=2;
        Require(!ValidateSnapshot(S),TEXT("invalid throw owner rejected"));
        S->Stones[0].PartyMember=0; S->Stones[0].Elapsed=S->Stones[0].Duration;
        Require(!ValidateSnapshot(S),TEXT("already completed saved flight rejected"));
        S->Stones.Reset(); S->Version=2; Require(ValidateSnapshot(S),TEXT("schema 2 remains readable without new state"));
        FOKSavedOrder NewOrder; NewOrder.Kind=EOKOrder::Whistle; S->Units[0].Orders.Add(NewOrder);
        Require(!ValidateSnapshot(S),TEXT("schema 2 cannot reinterpret newer whistle order")); S->Units[0].Orders.Reset();
        S->Version=1; Require(ValidateSnapshot(S),TEXT("schema 1 remains readable without new state"));
        Require(QuickSave(),TEXT("airborne flight written to isolated quicksave slot"));
        Next(); break;
    }
    case 1:
    {
        if (Now-StageStarted<.6) return;
        FVector2D Button;
        Require(HUD->ValidateLayout() && HUD->FindButton(TEXT("Whistle"),Button),TEXT("whistle HUD fits viewport and is actionable"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_DistractionPaused.png"),false,false);
        Next(); break;
    }
    case 2:
        if (Now-StageStarted<.3) return;
        Key(EKeys::F9);
        Require(GEngine->GetWorldContextFromWorldChecked(GetWorld()).TravelURL.Contains(TEXT("QuickLoad")),TEXT("F9 reconstructs mission with flight"));
        SmokeStage=99; break;
    case 99: return;
    case 100:
        Require(GetWorld()->IsPaused() && Stones.Num()==1 && Stones[0].Visual && FMath::IsNearlyEqual(Stones[0].Elapsed,.2f) &&
            Stones[0].Visual->GetActorLocation().Equals(Stones[0].Position(.2f),.1) && P->Distractions==4,
            TEXT("level reload restores visual, owner, flight clock and spent item without duplication"));
        AI->bBrainEnabled=true; TogglePause(); Next(); break;
    case 101:
        if (!Stones.IsEmpty() && Now-StageStarted<3) return;
        Require(Stones.IsEmpty(),TEXT("stone completes in real simulation time")); Next(); break;
    case 102:
        if (AI->AlertState!=EOKAlert::Investigate && Now-StageStarted<2) return;
        Require(AI->AlertState==EOKAlert::Investigate && FVector::Dist2D(AI->LastKnown,Throw.Location)<15,
            TEXT("UE hearing sends mobile patrol toward impact, not thrower"));
        Require(P->Distractions==4 && P->QueueSize()==0,TEXT("loaded flight cannot spend a second item"));
        Next(); break;
    case 103:
    {
        if (Now-StageStarted<.6) return;
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_DistractionInvestigate.png"),false,false);
        AI->bBrainEnabled=false; AI->StopMovement();
        TogglePause(); Place(Guard,FVector(880,1100,-8)); AI->Duty=EOKGuardDuty::HoldPost; AI->AlertState=EOKAlert::Patrol;
        P->Cooldown=0; Select(0); Key(EKeys::Y);
        Require(P->QueueSize()==1 && P->Distractions==4,TEXT("whistle uses no throwable inventory"));
        AI->bBrainEnabled=true; PausePosition=Guard->Feet(); TogglePause(); Next(); break;
    }
    case 104:
        if (AI->AlertState!=EOKAlert::Investigate && Now-StageStarted<2) return;
        Require(AI->AlertState==EOKAlert::Investigate && FVector::Dist2D(AI->LastKnown,P->Feet())<5 && P->Distractions==4,
            TEXT("whistle attracts at source location without consuming stones"));
        Next(); break;
    case 105:
    {
        if (Now-StageStarted<1) return;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_SENTINEL: displacement=%.2f duty=%d state=%d"),
            FVector::Dist2D(Guard->Feet(),PausePosition),int32(AI->Duty),int32(AI->AlertState));
        Require(FVector::Dist2D(Guard->Feet(),PausePosition)<2,TEXT("fixed sentry turns toward sound without abandoning post"));
        AI->bBrainEnabled=false; AI->StopMovement(); TogglePause(); P->CancelOrders(); P->Cooldown=0; O->Cooldown=1;
        ToggleCommandMode(); Throw.Location=FVector(400,1200,-8);
        Require(StagePlan(P,Whistling) && StagePlan(O,Throw) && ExecutePlan(),TEXT("mixed whistle and throw synchronize as team plan"));
        Next(); break;
    }
    case 106:
        if (Now-StageStarted<.25) return;
        Require(P->Cooldown==0 && P->QueueSize()==1 && O->Distractions==5 && Stones.IsEmpty(),TEXT("whistle waits at synchronization barrier for partner cooldown"));
        Next(); break;
    case 107:
    {
        if (ActiveSyncBatch && Now-StageStarted<3) return;
        Require(ActiveSyncBatch==0 && P->QueueSize()==0 && O->QueueSize()==0 && O->Distractions==4 && P->Cooldown>0,
            TEXT("synchronized whistle and throw release once without ending real-time mode"));
        Require(!bTacticalPause && !GetWorld()->IsPaused() && ValidateSnapshot(CaptureSnapshot()),TEXT("completed team action continues live simulation and remains saveable"));
        TogglePause(); Place(Guard,FVector(880,1100,-8)); AI->Duty=EOKGuardDuty::Patrol; AI->AlertState=EOKAlert::Combat;
        AI->Investigate(P->Feet()); Require(AI->AlertState==EOKAlert::Combat,TEXT("noise cannot downgrade combat into investigation"));
        AI->AlertState=EOKAlert::Patrol; AI->Suspicion=0; AI->bBrainEnabled=true;
        P->Cooldown=0; P->CancelOrders(); O->CancelOrders();
        for (auto& Stone:Stones) if (Stone.Visual) Stone.Visual->Destroy();
        Stones.Reset();
        Require(Distract(P,FVector(700,1100,-8)),TEXT("dynamic obstacle flight launches"));
        auto* Wall=Collider(FVector(550,1100,200),FVector(8,100,220),TEXT("StoneDynamicWall"));
        TogglePause(); UpdateStones(2); TogglePause();
        Require(Stones.IsEmpty() && NoisePulses.Last().Location.X<560,TEXT("new obstacle intercepts flight before intended landing"));
        Wall->GetOwner()->Destroy(); AI->bBrainEnabled=false; AI->StopMovement();
        Next(); break;
    }
    case 108:
        if (Now-StageStarted<.5) return;
        Require(HUD->ValidateLayout(),TEXT("loaded distraction HUD retains nonoverlapping bounds"));
        UE_LOG(LogTemp,Display,TEXT("OK_RT_DISTRACTION: PASS")); SmokeStage=-1;
        UKismetSystemLibrary::QuitGame(GetWorld(),PC,EQuitPreference::Quit,false); break;
    }
}
