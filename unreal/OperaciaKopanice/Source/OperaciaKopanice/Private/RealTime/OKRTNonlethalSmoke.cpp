#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTSaveGame.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTHUD.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include <limits>

void AOKRTGameMode::NonlethalSmokeTick()
{
    if (SmokeStage<0 || bInitialPausePending) return;
    const double Now=FPlatformTime::Seconds();
    if (Now-SmokeStarted>100) { Require(false,TEXT("nonlethal fixture timeout")); return; }
    auto Next=[&]() { if (SmokeStage>=0) { ++SmokeStage; StageStarted=Now; } };
    auto* P=Party[0].Get(); auto* O=Party[1].Get();
    auto* Body=Enemies[0].Get(); auto* Guard=Enemies[1].Get();
    auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController());
    auto* BodyAI=CastChecked<AOKRTGuardController>(Body->GetController());
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* HUD=CastChecked<AOKRTHUD>(PC->GetHUD());
    auto Place=[](AOKRTUnit* U,FVector Feet,float Yaw=0)
    {
        U->CancelOrders(); U->SetStance(EOKStance::Walk);
        U->SetActorLocation(Feet+FVector(0,0,U->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),false,nullptr,ETeleportType::TeleportPhysics);
        U->SetActorRotation(FRotator(0,Yaw,0)); U->GetCharacterMovement()->StopMovementImmediately();
    };
    auto Order=[Body](EOKOrder Kind) { FOKRTOrder V; V.Kind=Kind; V.Target=Body; V.Location=Body->Feet(); return V; };
    auto Key=[PC,this](FKey K)
    {
        for (EInputEvent Event:{IE_Pressed,IE_Released})
        {
            PC->InputKey(FInputKeyParams(K,Event,Event==IE_Pressed ? 1. : 0.));
            PC->PlayerInput->ProcessInputStack({PC->InputComponent},0,GetWorld()->IsPaused());
        }
    };
    switch (SmokeStage)
    {
    case 0:
    {
        Require(bTacticalPause && GetWorld()->IsPaused(),TEXT("nonlethal fixture starts with explicit tactical pause"));
        Require(FMath::IsNearlyEqual(float(Body->Visual->GetRelativeLocation().Z),-Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3),
            TEXT("rig pose and grounded visual initialize before any simulation tick"));
        Place(Body,FVector(520,1100,-8)); Place(P,FVector(620,1100,-8)); Place(O,FVector(200,1300,-8));
        FOKRTOrder Move; Move.Location=FVector(300,1200,-8);
        Require(P->Submit(Move),TEXT("baseline move queued"));
        Require(!P->Submit(Order(EOKOrder::Bind)) && !P->Submit(Order(EOKOrder::Carry)) && P->QueueSize()==1,
            TEXT("awake binding and carrying rejected without losing earlier orders"));
        P->CancelOrders(); Require(P->Submit(Order(EOKOrder::Knockout)),TEXT("knockout order accepted"));
        P->UpdateOrders();
        Require(Body->IsOperational() && Body->Health==100 && P->QueueSize()==0,TEXT("front approach cannot knock out a guard"));
        Place(P,FVector(400,1100,-8)); P->Cooldown=0;
        Key(EKeys::K); Require(PC->bArmed && PC->ArmedOrder==EOKOrder::Knockout,TEXT("pause-safe K arms nonlethal attack"));
        Key(EKeys::G); Require(PC->bArmed && PC->ArmedOrder==EOKOrder::Bind,TEXT("pause-safe G arms binding")); PC->bArmed=false;
        Require(P->Submit(Order(EOKOrder::Knockout)) && !P->Submit(Order(EOKOrder::Carry)),TEXT("knockout alone does not authorize carrying"));
        Require(P->Submit(Order(EOKOrder::Bind),true) && P->Submit(Order(EOKOrder::Carry),true) && P->QueueSize()==3,
            TEXT("typed knockout bind carry chain queues during pause"));
        P->Tick(0);
        Require(P->QueueSize()==3 && Body->IsOperational(),TEXT("already-scheduled paused unit tick cannot execute queued skills"));
        auto* S=CaptureSnapshot();
        Require(S->Version==2 && ValidateSnapshot(S),TEXT("schema 2 stores nonlethal queued orders"));
        S->Version=1; Require(!ValidateSnapshot(S),TEXT("schema 1 rejects newer order values"));
        S->Units[0].Orders.Reset(); Require(ValidateSnapshot(S),TEXT("legacy schema 1 remains readable with original states"));
        Next(); break;
    }
    case 1:
    {
        if (Now-StageStarted<.5) return;
        FVector2D Center;
        Require(HUD->ValidateLayout() && HUD->FindButton(TEXT("Knockout"),Center) && HUD->FindButton(TEXT("Bind"),Center),
            TEXT("nonlethal HUD controls fit and remain actionable"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_NonlethalPlan.png"),false,false);
        Next(); break;
    }
    case 2:
    {
        if (bTacticalPause)
        { if (Now-StageStarted<.2) return; TogglePause(); return; }
        if (!P->CarriedBody.IsValid() && Now-StageStarted<8) return;
        Require(Body->IsAlive() && !Body->IsOperational() && Body->bUnconscious && Body->bBound && Body->Health==100,
            TEXT("rear knockout and binding preserve enemy health"));
        Require(P->CarriedBody.Get()==Body && Body->Carrier.Get()==P && P->QueueSize()==0,TEXT("real-time chain carries bound enemy once"));
        Require(P->GetCharacterMovement()->MaxWalkSpeed<=100 && Body->GetCapsuleComponent()->GetCollisionEnabled()==ECollisionEnabled::NoCollision,
            TEXT("carrying slows carrier and disables carried collision"));
        P->SetStance(EOKStance::Run);
        Require(P->Stance==EOKStance::Walk,TEXT("carrying cannot retain running noise or running stance"));
        FOKRTOrder Run; Run.Kind=EOKOrder::Stance; Run.Stance=EOKStance::Run;
        Require(!P->Submit(Run) && P->QueueSize()==0,TEXT("running with a body is rejected before queue mutation"));
        bool Primary=false;
        Require(!Body->Vision->Sees(P,Primary) && !Guard->Vision->Sees(Body,Primary),TEXT("unconscious cannot see and carried cannot be discovered"));
        TogglePause();
        FOKRTOrder Drop; Drop.Kind=EOKOrder::Carry;
        FOKRTOrder Move; Move.Location=P->Feet()+FVector(0,100,0);
        Require(P->Submit(Drop,true) && P->Submit(Move,true) && P->PromoteLastMoveToRun(Move.Location) && P->QueueSize()==2,
            TEXT("run planned after dropping promotes the existing move without duplication"));
        P->CancelOrders();
        Require(QuickSave(),TEXT("bound carried enemy serializes to isolated slot"));
        auto* S=Cast<UOKRTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot(),0));
        Require(ValidateSnapshot(S) && S->Units[2].bBound && S->Units[0].CarriedBody==2,TEXT("disk snapshot preserves nonlethal ownership"));
        S->Units[2].bBound=false;
        Require(!ApplySnapshot(S) && Body->bBound && P->CarriedBody.Get()==Body,TEXT("unbound carried snapshot rejected without mutation"));
        S->Units[2].bBound=true; S->Units[2].RecoveryRemaining=std::numeric_limits<float>::quiet_NaN();
        Require(!ValidateSnapshot(S),TEXT("non-finite recovery clock rejected"));
        S->Units[2].RecoveryRemaining=0;
        Require(ApplySnapshot(S) && P->CarriedBody.Get()==Body && Body->bBound,TEXT("nonlethal pose and ownership restore from snapshot"));
        P->DropBody(true);
        Require(Body->bHiddenBody && Body->IsHidden() && Body->IsAlive() && !Body->WakeUp() && !Guard->Vision->Sees(Body,Primary),
            TEXT("hidden bound enemy cannot wake or be seen"));
        Require(ValidateSnapshot(CaptureSnapshot()),TEXT("hidden living bound enemy is a valid save state"));
        S->Units[0].CarriedBody=INDEX_NONE; S->Units[2].Feet=FVector(520,1100,-8);
        Require(ApplySnapshot(S) && !Body->bHiddenBody && Body->WakeUp(),TEXT("restored visible bound guard can be freed"));
        Require(Body->IsOperational() && Body->GetCharacterMovement()->MovementMode==MOVE_Walking &&
            FMath::IsNearlyEqual(Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),90.f),TEXT("recovery restores movement and standing capsule"));
        Require(Body->KnockOut(2),TEXT("unbound short recovery begins"));
        Next(); break;
    }
    case 3:
        if (Now-StageStarted<.6) return;
        Require(Body->bUnconscious && FMath::IsNearlyEqual(Body->RecoveryRemaining,2.f),TEXT("true tactical pause freezes recovery clock"));
        TogglePause(); Next(); break;
    case 4:
    {
        if (Body->bUnconscious && Now-StageStarted<4) return;
        Require(Body->IsOperational() && !Body->bBound && BodyAI->AlertState==EOKAlert::Investigate,
            TEXT("unbound guard recovers in continuous time and investigates"));
        TogglePause(); Place(P,FVector(400,1100,-8)); P->Cooldown=0;
        Require(Body->KnockOut(45),TEXT("second knockout accepted after recovery"));
        Require(!P->Submit(Order(EOKOrder::Carry)) && !P->Submit(Order(EOKOrder::Knockout)),
            TEXT("unbound unconscious cannot be carried or repeatedly hit"));
        FVector2D Screen;
        Require(PC->ProjectWorldLocationToScreen(Body->GetActorLocation(),Screen) && PC->CommandAt(Screen),
            TEXT("unarmed LMB unconscious enemy issues contextual binding"));
        FOKRTOrder Pending; Require(P->GetQueuedOrder(0,Pending) && Pending.Kind==EOKOrder::Bind,TEXT("context resolves binding rather than lethal attack"));
        P->CancelOrders(); Require(Body->BindBody() && !Body->BindBody(),TEXT("binding is single-use"));
        auto* S=CaptureSnapshot(); S->Units[2].bUnconscious=false;
        Require(!ValidateSnapshot(S),TEXT("bound conscious impossible state rejected"));
        ToggleCommandMode();
        Require(StagePlan(P,Order(EOKOrder::Carry)) && StagePlan(O,Order(EOKOrder::Carry)) && !ExecutePlan(),
            TEXT("synchronized duplicate body access is atomic"));
        CancelPlan();
        Place(P,FVector(1700,1400,-8)); Place(O,FVector(1550,1400,-8));
        Place(Guard,FVector(720,1100,-8),180);
        AI->DiscoveredBodies.Reset(); AI->RescueTarget.Reset(); AI->AlertState=EOKAlert::Patrol; AI->Suspicion=0;
        AI->Duty=EOKGuardDuty::HoldPost; AI->bBrainEnabled=true;
        TogglePause(); AI->Tick(.1f); TogglePause();
        Require(AI->AlertState==EOKAlert::Investigate && !AI->RescueTarget.IsValid() && Body->bBound,
            TEXT("fixed sentry observes casualty without abandoning post"));
        AI->Duty=EOKGuardDuty::Patrol; AI->DiscoveredBodies.Reset(); AI->AlertState=EOKAlert::Patrol;
        TogglePause(); AI->Tick(.1f); TogglePause();
        Require(AI->RescueTarget.Get()==Body && Body->bBound,TEXT("mobile guard remembers casualty but cannot free remotely"));
        auto* Rescue=CaptureSnapshot();
        Require(ValidateSnapshot(Rescue) && Rescue->Guards[1].RescueTarget==0 && ApplySnapshot(Rescue) && AI->RescueTarget.Get()==Body,
            TEXT("rescue approach survives snapshot restoration"));
        TogglePause(); Next(); break;
    }
    case 5:
    {
        if (Body->bUnconscious && Now-StageStarted<8) return;
        Require(Body->IsOperational() && !Body->bBound && FVector::Dist2D(Guard->Feet(),Body->Feet())<=145,
            TEXT("mobile guard frees ally only after real NavMesh approach"));
        AI->bBrainEnabled=false; AI->StopMovement(); TogglePause();
        Place(Body,FVector(520,1100,-8)); Require(Body->KnockOut(45) && Body->BindBody(),TEXT("bound casualty prepared for obstruction check"));
        Place(Guard,FVector(620,1100,-8),180);
        auto* Wall=Collider(FVector(570,1100,100),FVector(8,120,120),TEXT("NonlethalFixtureWall"));
        AI->RescueTarget=Body; AI->AlertState=EOKAlert::Investigate; AI->bBrainEnabled=true;
        TogglePause(); AI->Tick(.1f); TogglePause();
        Require(Body->bBound && Body->bUnconscious,TEXT("wall prevents freeing through nearby geometry"));
        AI->bBrainEnabled=false; AI->StopMovement(); Wall->GetOwner()->Destroy();
        Place(P,FVector(520,1000,-8),90); P->Cooldown=0;
        Require(P->Submit(Order(EOKOrder::Bind))==false,TEXT("already bound target rejects duplicate player binding"));
        auto* S=CaptureSnapshot(); S->Guards[1].RescueTarget=INDEX_NONE;
        Require(ApplySnapshot(S),TEXT("stable casualty snapshot reapplies after obstruction fixture"));
        Require(TeamSees(Body),TEXT("bound casualty indicator uses real party line of sight"));
        Select(0); P->CancelOrders();
        Require(QuickSave(),TEXT("bound on-ground casualty saved before level reload"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_NonlethalBound.png"),false,false);
        Next(); break;
    }
    case 6:
        if (Now-StageStarted<.6) return;
        Key(EKeys::F9);
        Require(GEngine->GetWorldContextFromWorldChecked(GetWorld()).TravelURL.Contains(TEXT("QuickLoad")),
            TEXT("F9 reconstructs saved nonlethal mission"));
        SmokeStage=99; break;
    case 99: return;
    case 100:
    {
        Require(Body->bUnconscious && Body->bBound && Body->Health==100 && !Body->Carrier.IsValid() && GetWorld()->IsPaused(),
            TEXT("level reconstruction restores bound on-ground state and tactical pause"));
        Require(Body->WakeUp() && Body->KnockOut(1) && Body->BindBody(),TEXT("loaded casualty can recover and be bound again"));
        TogglePause(); Body->Tick(10); TogglePause();
        Require(Body->bUnconscious && Body->bBound && Body->RecoveryRemaining==0,TEXT("bound enemy never self-recovers"));
        Body->TakeHit(100);
        Require(!Body->IsAlive() && !Body->bUnconscious && !Body->bBound && ValidateSnapshot(CaptureSnapshot()),
            TEXT("lethal damage clears nonlethal state and keeps corpse compatibility"));
        Require(!Body->WakeUp() && !Body->KnockOut(),TEXT("dead enemies cannot revive or be knocked out"));
        Next(); break;
    }
    case 101:
        if (Now-StageStarted<.5) return;
        Require(HUD->ValidateLayout(),TEXT("loaded HUD retains safe action bounds"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_NonlethalLoaded.png"),false,false);
        Next(); break;
    case 102:
        if (Now-StageStarted<.6) return;
        Require(UGameplayStatics::DeleteGameInSlot(SaveSlot(),0),TEXT("fixture removes isolated save slot"));
        UE_LOG(LogTemp,Display,TEXT("OK_RT_NONLETHAL: PASS"));
        FPlatformMisc::RequestExit(false); bNonlethalSmoke=bSmoke=false; break;
    }
}
