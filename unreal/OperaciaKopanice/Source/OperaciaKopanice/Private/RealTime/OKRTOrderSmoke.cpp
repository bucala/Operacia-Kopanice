#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTHUD.h"
#include "AIController.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "UnrealClient.h"

void AOKRTGameMode::SmokeOrderTests()
{
    auto* Unit=Party[0].Get();
    TArray<FOKRTOrder> Saved;
    FOKRTOrder Order;
    for (int32 I=0;Unit->GetQueuedOrder(I,Order);++I) Saved.Add(Order);
    Require(Saved.Num()==2 && Saved[0].Kind==EOKOrder::Move && Saved[1].Kind==EOKOrder::Stance,
        TEXT("order inspection exposes typed movement and stance in execution order"));
    Order.Location=FVector(1,2,3);
    Require(!Unit->GetQueuedOrder(-1,Order) && !Unit->GetQueuedOrder(32,Order) && Order.Location==FVector(1,2,3),
        TEXT("invalid queue indices do not mutate order snapshots"));
    Unit->GetQueuedOrder(0,Order); Order.Kind=EOKOrder::Distract;
    FOKRTOrder Retained; Unit->GetQueuedOrder(0,Retained);
    Require(Retained.Kind==EOKOrder::Move,TEXT("HUD order snapshots cannot edit the production queue"));
    Require(Unit->UndoLastOrder() && Unit->QueueSize()==1 && Unit->GetQueuedOrder(0,Order) && Order.Kind==EOKOrder::Move,
        TEXT("paused undo removes only the most recent waiting order"));
    const int32 Charges=Unit->Distractions; const float Cooldown=Unit->Cooldown;
    Unit->Submit(Saved[1],true);
    bTacticalPause=false;
    Require(!Unit->CanUndoLastOrder() && !Unit->UndoLastOrder() && Unit->QueueSize()==2,
        TEXT("real-time simulation cannot undo queued orders"));
    bTacticalPause=true;
    UGameplayStatics::SetGamePaused(GetWorld(),false);
    Require(!Unit->UndoLastOrder(),TEXT("order undo requires a real paused world not just the tactical flag"));
    UGameplayStatics::SetGamePaused(GetWorld(),true);
    ToggleMenu();
    Require(!Unit->UndoLastOrder() && Unit->QueueSize()==2,TEXT("menu pause cannot edit tactical orders"));
    ToggleMenu();
    bWon=true;
    Require(!Unit->UndoLastOrder(),TEXT("mission results block order undo")); bWon=false;
    Unit->Health=0;
    Require(!Unit->UndoLastOrder(),TEXT("dead units cannot edit an order queue")); Unit->Health=100;
    Unit->bEnemy=true;
    Require(!Unit->UndoLastOrder(),TEXT("enemy units cannot use party order undo")); Unit->bEnemy=false;
    Unit->CancelOrders(); Unit->Submit(Saved[0],true);
    Unit->Tick(0);
    auto* Path=CastChecked<AAIController>(Unit->GetController())->GetPathFollowingComponent();
    const auto Request=Path->GetCurrentRequestId();
    Require(Unit->HasStartedOrder() && !Unit->UndoLastOrder() && Unit->QueueSize()==1,
        TEXT("paused undo protects an already-started movement order"));
    Unit->Submit(Saved[1],true);
    Require(Unit->UndoLastOrder() && Unit->QueueSize()==1 && Path->GetCurrentRequestId()==Request && Unit->HasStartedOrder(),
        TEXT("tail undo preserves the active UE path-following request"));
    Require(Unit->PromoteLastMoveToRun(Saved[0].Location) && !Unit->UndoLastOrder(),
        TEXT("double-click pace promotion cannot make an active path undoable"));
    Unit->CancelOrders();
    Require(!Unit->CanUndoLastOrder() && !Unit->UndoLastOrder(),TEXT("empty order queues safely reject undo"));
    Require(Unit->Distractions==Charges && Unit->Cooldown==Cooldown,
        TEXT("queue inspection and undo spend no charges or cooldown time"));
    for (const auto& Pending:Saved) Unit->Submit(Pending,true);
    SelectAll();
}

bool AOKRTGameMode::SmokeOrderHUDTests()
{
    if (OrderHUDSmokeStage<0) return true;
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* UI=CastChecked<AOKRTHUD>(PC->GetHUD());
    auto* Unit=Party[0].Get();
    FVector2D P=FVector2D::ZeroVector;
    auto Button=[&](FName Id)
    { Require(UI->FindButton(Id,P),*FString::Printf(TEXT("tactical queue exposes %s"),*Id.ToString())); return SmokeStage>=0; };
    if (OrderHUDSmokeStage==0)
    {
        const int32 Other=Party[1]->QueueSize();
        Require(UI->ValidateLayout(),TEXT("tactical queue respects portrait and desktop HUD bounds"));
        if (!Button(TEXT("QueuedOrder1"))) return false;
        Require(UI->Click(P) && Unit->QueueSize()==2,TEXT("order inspection leaves the queue unchanged"));
        Require(!PC->CommandAt(P) && Unit->QueueSize()==2,TEXT("queue UI rejects terrain orders through its panel"));
        if (!Button(TEXT("UndoOrder"))) return false;
        PC->Arm(EOKOrder::Distract);
        Require(PC->TapAt(P) && Unit->QueueSize()==1 && Party[1]->QueueSize()==Other && !PC->bArmed,
            TEXT("touch undo edits only the active specialist and cancels targeting"));
        Unit->CancelOrders();
        FOKRTOrder Stance; Stance.Kind=EOKOrder::Stance; Stance.Stance=EOKStance::Crouch;
        bool Accepted=true;
        for (int32 I=0;I<32;++I) Accepted&=Unit->Submit(Stance,true);
        Require(Accepted && !Unit->Submit(Stance,true) && Unit->QueueSize()==32,TEXT("queue preview retains the thirty-two-order production limit"));
        ++OrderHUDSmokeStage; return false;
    }
    if (OrderHUDSmokeStage>=1 && OrderHUDSmokeStage<=6)
    {
        const int32 First=(OrderHUDSmokeStage-1)*5;
        Require(UI->ValidateLayout() && UI->FindButton(FName(*FString::Printf(TEXT("QueuedOrder%d"),First)),P),
            TEXT("paged tactical queue keeps its current order range reachable"));
        if (!Button(TEXT("QueueNext"))) return false;
        UI->Click(P);
        if (OrderHUDSmokeStage==6) FScreenshotRequest::RequestScreenshot(TEXT("OKRT_OrderQueue.png"),false,false);
        ++OrderHUDSmokeStage; return false;
    }
    if (OrderHUDSmokeStage==7)
    {
        Require(UI->ValidateLayout() && UI->FindButton(TEXT("QueuedOrder31"),P),TEXT("tactical queue reaches its final thirty-second order"));
        if (!Button(TEXT("QueueNext"))) return false;
        UI->Click(P);
        PC->InputKey(FInputKeyParams(EKeys::BackSpace,IE_Pressed,1.0));
        PC->PlayerInput->ProcessInputStack({PC->InputComponent},0,true);
        PC->InputKey(FInputKeyParams(EKeys::BackSpace,IE_Released,0.0));
        PC->PlayerInput->ProcessInputStack({PC->InputComponent},0,true);
        Require(Unit->QueueSize()==31,TEXT("pause-enabled Backspace removes one waiting order"));
        while (Unit->QueueSize()>2) Unit->UndoLastOrder();
        ++OrderHUDSmokeStage; return false;
    }
    if (OrderHUDSmokeStage==8)
    {
        Require(UI->FindButton(TEXT("QueuedOrder0"),P) && !UI->FindButton(TEXT("QueuedOrder5"),P) && UI->ValidateLayout(),
            TEXT("shrinking queues clamp the displayed page without stale hit regions"));
        Select(1,true); ++OrderHUDSmokeStage; return false;
    }
    if (OrderHUDSmokeStage==9)
    {
        Require(!UI->FindButton(TEXT("UndoOrder"),P),TEXT("switching to an idle specialist removes the old queue panel"));
        Select(0,true);
        Unit->CancelOrders();
        FOKRTOrder Move; Move.Location=FVector(810,1260,0); Unit->Submit(Move,true);
        FOKRTOrder Stance; Stance.Kind=EOKOrder::Stance; Stance.Stance=EOKStance::Crouch; Unit->Submit(Stance,true);
        ++OrderHUDSmokeStage; return false;
    }
    Require(UI->FindButton(TEXT("QueuedOrder0"),P) && UI->ValidateLayout(),TEXT("restored mission plan redraws after switching specialists"));
    FScreenshotRequest::RequestScreenshot(TEXT("OKRT_OrderPlan.png"),false,false);
    OrderHUDSmokeStage=-1; return false;
}
