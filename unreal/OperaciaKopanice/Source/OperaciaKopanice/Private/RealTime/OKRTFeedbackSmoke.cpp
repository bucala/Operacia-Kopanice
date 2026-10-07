#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTPlayerController.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include <limits>

void AOKRTGameMode::SmokeFeedbackTests()
{
    TArray<FOKRTOrder> Saved[2];
    FOKRTOrder Order;
    for (int32 Member=0;Member<2;++Member)
        for (int32 I=0;Party[Member]->GetQueuedOrder(I,Order);++I) Saved[Member].Add(Order);
    const FString SavedMessage=Message;
    const auto SavedFeedback=CommandFeedback;
    Require(!Saved[0].IsEmpty(),TEXT("feedback fixture has a pending movement origin"));
    if (Saved[0].IsEmpty()) return;
    Select(0);
    auto* Unit=Party[0].Get();
    const int32 Charges=Unit->Distractions; const float Cooldown=Unit->Cooldown;
    FOKRTOrder Move; Move.Location=FVector(720,1260,0);
    TArray<FVector> Points; FString Reason;
    Require(Unit->GetOrderPath(Move,true,Points,Reason) && Points.Num()>=2 &&
        FVector::Dist2D(Points[0],Saved[0][0].Location)<30,
        TEXT("paused preview starts at the last queued destination"));
    FOKRTOrder Water; Water.Location=FVector(1080,1260,0);
    Require(!PreviewOrder(Water) && PreviewSpline->GetNumberOfSplinePoints()==0 && Message==SavedMessage,
        TEXT("hovering water cannot display a projected bank route or mutate feedback"));
    FOKRTOrder Above; Above.Location=FVector(720,1260,500);
    Require(!Unit->GetOrderPath(Above,false,Points,Reason) && Points.IsEmpty(),TEXT("preview rejects an elevated projected endpoint"));
    FOKRTOrder Cabin; Cabin.Location=FVector(450,270,0);
    Require(!PreviewOrder(Cabin),TEXT("preview rejects a cabin interior without navigation"));
    FOKRTOrder Missing; Missing.Kind=EOKOrder::Takedown;
    Require(!Unit->GetOrderPath(Missing,false,Points,Reason),TEXT("preview handles a missing target without dereferencing it"));
    Require(PreviewOrder(Move) && Unit->QueueSize()==Saved[0].Num() && Unit->Distractions==Charges,
        TEXT("valid route preview is read-only"));
    Require(Command(Move) && CommandFeedback.Accepted==1 && CommandFeedback.Requested==1 &&
        CommandFeedback.Location==Move.Location && CommandFeedbackOpacity()>0 && Message.Contains(Unit->DisplayName),
        TEXT("accepted world command records its target and active specialist"));
    const int32 Count=Unit->QueueSize();
    Require(!Command(Water) && CommandFeedback.Accepted==0 && CommandFeedback.Requested==1 && Unit->QueueSize()==Count &&
        Message.Contains(TEXT("pristupnu")),TEXT("rejected world command explains failure and retains the existing plan"));
    bPathPreview=false;
    Require(!PreviewOrder(Move) && Command(Move) && CommandFeedbackOpacity()>0,
        TEXT("command acknowledgement remains available when route previews are disabled"));
    bPathPreview=true;
    Party[1]->CancelOrders();
    FOKRTOrder Stance; Stance.Kind=EOKOrder::Stance; Stance.Stance=EOKStance::Crouch;
    for (int32 I=0;I<32;++I) Party[1]->Submit(Stance,true);
    SelectAll();
    const int32 Before=Unit->QueueSize();
    Require(Command(Move) && CommandFeedback.Accepted==1 && CommandFeedback.Requested==2 &&
        Unit->QueueSize()==Before+1 && Party[1]->QueueSize()==32 && Message.Contains(TEXT("1/2")),
        TEXT("partially accepted group command reports both outcomes without clearing the rejected queue"));
    Require(PreviewOrder(Move) && PreviewSpline->GetNumberOfSplinePoints()>=2 &&
        FVector::Dist2D(PreviewSpline->GetLocationAtSplinePoint(PreviewSpline->GetNumberOfSplinePoints()-1,ESplineCoordinateSpace::World),
            IndividualOrder(Move,Unit).Location)<30,TEXT("group hover preview matches the active unit formation offset"));
    for (int32 I=Unit->QueueSize();I<32;++I) Unit->Submit(Stance,true);
    Require(!Command(Move) && CommandFeedback.Accepted==0 && CommandFeedback.Requested==2 && Unit->QueueSize()==32 && Party[1]->QueueSize()==32,
        TEXT("fully rejected group command preserves both full queues"));
    const double Issued=CommandFeedback.IssuedAt;
    ToggleMenu();
    Require(!Command(Move) && CommandFeedback.IssuedAt==Issued && CommandFeedbackOpacity()==0,
        TEXT("menu gating cannot emit a world acknowledgement"));
    ToggleMenu();
    bWon=true;
    Require(!Command(Move) && CommandFeedbackOpacity()==0,TEXT("results suppress command acknowledgement")); bWon=false;
    CommandFeedback.IssuedAt=FPlatformTime::Seconds()-3;
    Require(CommandFeedbackOpacity()==0,TEXT("acknowledgement expires on real time even during tactical pause"));
    CommandFeedback.IssuedAt=FPlatformTime::Seconds();
    CommandFeedback.Location.X=std::numeric_limits<double>::quiet_NaN();
    Require(CommandFeedbackOpacity()==0,TEXT("non-finite world targets cannot draw an acknowledgement"));
    Require(Unit->Distractions==Charges && Unit->Cooldown==Cooldown,TEXT("preview and command acknowledgements do not spend skill resources"));
    for (int32 Member=0;Member<2;++Member)
    {
        Party[Member]->CancelOrders();
        for (const auto& Pending:Saved[Member]) Party[Member]->Submit(Pending,true);
    }
    SelectAll(); Message=SavedMessage; CommandFeedback=SavedFeedback; PreviewSpline->ClearSplinePoints();
}

bool AOKRTGameMode::SmokeFeedbackHUDTests()
{
    if (FeedbackHUDSmokeStage<0) return true;
    auto* UI=CastChecked<AOKRTHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
    FVector2D P=FVector2D::ZeroVector;
    if (FeedbackHUDSmokeStage==0)
    {
        Select(0);
        FOKRTOrder Water; Water.Location=FVector(1080,1260,0);
        Require(!Command(Water) && Party[0]->QueueSize()==2,TEXT("rendered rejection keeps the planned movement and stance"));
        CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController())->Arm(EOKOrder::Takedown);
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_CommandRejected.png"),false,false);
        ++FeedbackHUDSmokeStage; return false;
    }
    if (FeedbackHUDSmokeStage==1)
    {
        Require(UI->FindCommandFeedback(P) && !UI->OverUI(P) && !UI->Click(P) && UI->ValidateLayout(),
            TEXT("rejection acknowledgement renders outside HUD controls without intercepting input"));
        auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
        Require(PC->bArmed,TEXT("armed targeting remains available while rejection feedback is rendered"));
        PC->bArmed=false;
        FOKRTOrder Move; Move.Location=FVector(720,1260,0);
        Require(Command(Move) && CommandFeedback.Accepted==1,TEXT("rendered accepted movement records success during pause"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_CommandAccepted.png"),false,false);
        ++FeedbackHUDSmokeStage; return false;
    }
    Require(UI->FindCommandFeedback(P) && UI->ValidateLayout(),TEXT("accepted acknowledgement has valid responsive screen bounds"));
    Require(Party[0]->UndoLastOrder() && Party[0]->QueueSize()==2,TEXT("feedback fixture restores the original pending mission plan"));
    CommandFeedback={}; Message=TEXT("Takticka pauza"); SelectAll();
    FeedbackHUDSmokeStage=-1; return false;
}
