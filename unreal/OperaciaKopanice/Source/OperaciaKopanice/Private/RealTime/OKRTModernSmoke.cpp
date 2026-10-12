#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTSaveGame.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTHUD.h"
#include "Navigation/OKViaductActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerInput.h"
#include "UnrealClient.h"
#include <limits>

void AOKRTGameMode::ModernSmokeTick()
{
    if (SmokeStage<0 || bInitialPausePending) return;
    const double Now=FPlatformTime::Seconds();
    if (Now-SmokeStarted>100) { Require(false,TEXT("modern tactics fixture timeout")); return; }
    auto Next=[&]() { if (SmokeStage<0) return; ++SmokeStage; StageStarted=Now; };
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* HUD=CastChecked<AOKRTHUD>(PC->GetHUD());
    auto* P=Party[0].Get(); auto* O=Party[1].Get();
    auto Key=[PC,this](FKey K)
    {
        for (EInputEvent Event:{IE_Pressed,IE_Released})
        {
            PC->InputKey(FInputKeyParams(K,Event,Event==IE_Pressed ? 1. : 0.));
            PC->PlayerInput->ProcessInputStack({PC->InputComponent},0,GetWorld()->IsPaused());
        }
    };
    auto DraftMove=[&](int32 Member,FVector Destination)
    { FOKRTOrder Order; Order.Location=Destination; return StagePlan(Party[Member],Order); };
    switch (SmokeStage)
    {
    case 0:
    {
        Require(bTacticalPause && !bMenu,TEXT("modern fixture explicitly pauses; production startup tested separately"));
        Require(P->Specialist==EOKSpecialist::Partisan && O->Specialist==EOKSpecialist::Officer && P->Medkits==0 && O->Medkits==2,TEXT("specialist equipment assigned by role"));
        FOKRTOrder Old; Old.Location=FVector(720,1260,0);
        Require(P->Submit(Old),TEXT("existing ordinary order accepted"));
        Key(EKeys::Z);
        Require(bCommandMode,TEXT("Z hotkey opens team planning"));
        Require(DraftMove(0,FVector(720,1260,0)) && DraftMove(1,FVector(720,1380,0)),TEXT("independent team drafts accepted"));
        if (SmokeStage<0) return;
        Require(P->QueueSize()==1 && O->QueueSize()==0,TEXT("editing drafts leaves existing orders untouched"));
        auto* S=CaptureSnapshot();
        if (HasQuickSave()) UGameplayStatics::DeleteGameInSlot(SaveSlot(),0);
        Key(EKeys::F5);
        auto* Stored=Cast<UOKRTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot(),0));
        Require(ValidateSnapshot(Stored) && Stored->bCommandMode && Stored->Drafts.Num()==2 &&
            Stored->DraftValid==S->DraftValid && Stored->Units[0].Orders.Num()==1,
            TEXT("F5 serializes draft plan to isolated test save slot"));
        if (SmokeStage<0) return;
        S->Version=999;
        Require(!ApplySnapshot(S) && bCommandMode && P->QueueSize()==1,TEXT("unknown schema rejected without mutation"));
        S->Version=1; S->Drafts[0].Target=999;
        Require(!ValidateSnapshot(S),TEXT("invalid actor references rejected")); S->Drafts[0].Target=INDEX_NONE;
        CancelPlan();
        Require(bTacticalPause && P->QueueSize()==1 && !bCommandMode,TEXT("cancel restores prior pause and original orders"));
        Require(ApplySnapshot(S) && bCommandMode,TEXT("snapshot restores drafts and ordinary queues"));
        Next(); break;
    }
    case 1:
        if (Now-StageStarted<.7) return;
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_ModernPlan.png"),false,false);
        Next(); break;
    case 2:
    {
        if (Now-StageStarted<.2) return;
        FVector2D Center;
        Require(HUD->ValidateLayout() && HUD->FindButton(TEXT("ExecutePlan"),Center) && HUD->FindButton(TEXT("Draft1"),Center),TEXT("command HUD has non-overlapping actionable controls"));
        const auto Map=HUD->GetTacticalMapBounds();
        if (Map.bIsValid)
        {
            const auto CameraBefore=GetCameraTarget(); const auto Expected=TacticalBounds().GetCenter();
            Require(HUD->Click(Map.GetCenter()) && FVector2D(GetCameraTarget()).Equals(Expected,1) && P->QueueSize()==1,
                TEXT("minimap focuses camera without issuing unit orders"));
            FocusWorld(CameraBefore);
        }
        CancelPlan(); P->CancelOrders(); O->CancelOrders(); ToggleCommandMode();
        FOKRTOrder Attack; Attack.Kind=EOKOrder::Takedown; Attack.Target=Enemies[0]; Attack.Location=Enemies[0]->Feet();
        Require(StagePlan(P,Attack) && StagePlan(O,Attack) && !ExecutePlan() && P->QueueSize()==0 && O->QueueSize()==0,TEXT("duplicate target prevents partial synchronization commit"));
        CancelPlan(); ToggleCommandMode();
        FOKRTOrder Far; Far.Kind=EOKOrder::Distract; Far.Location=FVector(10000,10000,0);
        Require(!StagePlan(P,Far),TEXT("out-of-range synchronized throw rejected before commit"));
        FOKRTOrder A; A.Kind=EOKOrder::Distract; A.Location=FVector(400,1200,0);
        FOKRTOrder B=A; B.Location=FVector(450,1190,0);
        P->Cooldown=0; O->Cooldown=1.3f;
        Require(StagePlan(P,A) && StagePlan(O,B) && ExecutePlan() && !GetWorld()->IsPaused(),TEXT("team plan resumes continuous simulation"));
        Next(); break;
    }
    case 3:
        if (Now-StageStarted<.3) return;
        Require(P->Distractions==5 && O->Distractions==5 && ActiveSyncBatch!=0,TEXT("ready member waits for partner cooldown without spending ammo"));
        { auto* S=CaptureSnapshot(); Require(S->ActiveBatch!=0 && ApplySnapshot(S) && ActiveSyncBatch!=0,TEXT("pending execution barrier survives snapshot restore")); }
        Next(); break;
    case 4:
        if (Now-StageStarted<1.7) return;
        Require(P->Distractions==4 && O->Distractions==4 && ActiveSyncBatch==0 && P->QueueSize()==0 && O->QueueSize()==0,TEXT("barrier releases both actions once"));
        Require(!bTacticalPause && !GetWorld()->IsPaused(),TEXT("batch completion never auto-pauses real time"));
        TogglePause(); P->Health=40; O->Cooldown=0; Select(0);
        { FOKRTOrder Heal; Heal.Kind=EOKOrder::Heal; Heal.Target=P; Heal.Location=P->Feet();
          Require(!P->Submit(Heal),TEXT("partisan cannot use officer medical equipment"));
          Select(1); Key(EKeys::H); FVector2D Marker;
          Require(PC->bArmed && HUD->FindPartyMarker(0,Marker) && PC->CommandAt(Marker),TEXT("H plus ally marker issues officer healing")); }
        TogglePause(); Next(); break;
    case 5:
        if (P->Health<100 && Now-StageStarted<8) return;
        Require(P->Health==100 && O->Medkits==1 && O->Cooldown>0,TEXT("healing restores health and consumes one medkit"));
        TogglePause();
        { FOKRTOrder Heal; Heal.Kind=EOKOrder::Heal; Heal.Target=P; Heal.Location=P->Feet();
          Require(!O->Submit(Heal),TEXT("full-health target rejected")); }
        P->CancelOrders(); O->CancelOrders(); ToggleCommandMode();
        { FOKRTOrder A; A.Kind=EOKOrder::Takedown; A.Target=Enemies[0]; A.Location=Enemies[0]->Feet();
          FOKRTOrder B; B.Location=FVector(720,1380,0);
          Require(StagePlan(P,A) && StagePlan(O,B) && ExecutePlan(),TEXT("mixed approach and action committed"));
          Enemies[0]->TakeHit(100); }
        Next(); break;
    case 6:
    {
        if (Now-StageStarted<.4) return;
        Require(ActiveSyncBatch==0 && P->QueueSize()==0 && O->QueueSize()==0,TEXT("lost target cancels unreleased batch"));
        TogglePause(); P->SetStance(EOKStance::Walk); O->SetStance(EOKStance::Walk);
        P->SetActorLocation(FVector(1500,1100,90)); O->SetActorLocation(FVector(1640,1100,90));
        P->Health=40; P->Cooldown=0; O->Cooldown=0;
        bHasTNT=true; BreakBridge();
        auto* AI=CastChecked<AOKRTGuardController>(Enemies[1]->GetController());
        AI->AlertState=EOKAlert::Investigate; AI->Suspicion=.42f; AI->LastKnown=FVector(1600,900,0); AI->Wait=2.75f;
        auto* Body=Enemies[0].Get(); P->CarriedBody=Body; Body->Carrier=P;
        Body->GetCapsuleComponent()->SetCollisionProfileName(TEXT("NoCollision"));
        Body->AttachToActor(P,FAttachmentTransformRules::KeepWorldTransform); Body->SetActorRelativeLocation(FVector(-35,0,50)); P->SetStance(P->Stance);
        CameraTarget=FVector(1550,850,0); CameraYaw=42; UpdateCamera(); ToggleCommandMode();
        Require(DraftMove(0,FVector(1560,950,0)),TEXT("post-sabotage escape draft accepted"));
        FOKRTOrder Heal; Heal.Kind=EOKOrder::Heal; Heal.Target=P; Heal.Location=P->Feet();
        Require(StagePlan(O,Heal) && QuickSave(),TEXT("save contains wreck corpse guard state and drafts"));
        auto* S=CaptureSnapshot(); S->Units[0].Feet.X=std::numeric_limits<double>::quiet_NaN();
        Require(!ApplySnapshot(S) && P->Health==40 && bCommandMode,TEXT("non-finite coordinates cannot mutate live world"));
        Key(EKeys::F9);
        Require(GEngine->GetWorldContextFromWorldChecked(GetWorld()).TravelURL.Contains(TEXT("QuickLoad")),TEXT("F9 requests native saved-mission reassembly"));
        SmokeStage=99; StageStarted=Now; break;
    }
    case 99: return;
    case 100:
    {
        auto* AI=CastChecked<AOKRTGuardController>(Enemies[1]->GetController()); FOKRTOrder A,B;
        Require(bBridgeDestroyed && bHasTNT && bCommandMode && bTacticalPause,TEXT("load restores topology and planning state"));
        Require(Bridge && Bridge->Tags.Contains(TEXT("RustboundBridge")) && Bridge->bDestroyed && Bridge->IsDemoModelConsistent(),
            TEXT("loaded sabotage swaps supplied span for fractured asset and preserves piers"));
        Require(P->Health==40 && O->Medkits==1 && P->CarriedBody.Get()==Enemies[0] && Enemies[0]->Carrier.Get()==P,TEXT("load restores specialist inventory and corpse ownership"));
        Require(AI->AlertState==EOKAlert::Investigate && FMath::IsNearlyEqual(AI->Suspicion,.42f) && FMath::IsNearlyEqual(AI->Wait,2.75f),TEXT("load restores suspicion and FSM timers"));
        Require(GetDraft(0,A) && GetDraft(1,B) && B.Kind==EOKOrder::Heal && B.Target.Get()==P && FMath::IsNearlyEqual(CameraYaw,42.f),TEXT("load resolves draft references and camera"));
        Next(); break;
    }
    case 101:
        if (Now-StageStarted<.7) return;
        Require(!HasCompletePath(FVector(720,720,0),DetonatorLocation),TEXT("loaded wreck disconnects river navigation"));
        Require(HUD->ValidateLayout(),TEXT("loaded plan HUD remains interactive"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_ModernLoaded.png"),false,false); Next(); break;
    case 102:
        if (Now-StageStarted<.7) return;
        Require(ExecutePlan(),TEXT("loaded team plan executes")); Next(); break;
    case 103:
        if ((P->Health<100 || ActiveSyncBatch!=0) && Now-StageStarted<8) return;
        Require(P->Health==100 && O->Medkits==0 && ActiveSyncBatch==0,TEXT("loaded synchronization executes once with restored inventory"));
        OpenMenuPage(EOKMenuPage::Pause); Next(); break;
    case 104:
        if (Now-StageStarted<.5) return;
        { FVector2D Center; Require(HUD->ValidateLayout() && HUD->FindButton(TEXT("SaveMission"),Center) && HUD->FindButton(TEXT("LoadMission"),Center),TEXT("save and load menu rows fit viewport")); }
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_ModernMenu.png"),false,false); Next(); break;
    case 105:
        if (Now-StageStarted<.7) return;
        Require(UGameplayStatics::DeleteGameInSlot(SaveSlot(),0),TEXT("automation cleans isolated save slot"));
        UE_LOG(LogTemp,Display,TEXT("OK_RT_MODERN: PASS"));
        FPlatformMisc::RequestExit(false); bModernSmoke=bSmoke=false; break;
    }
}
