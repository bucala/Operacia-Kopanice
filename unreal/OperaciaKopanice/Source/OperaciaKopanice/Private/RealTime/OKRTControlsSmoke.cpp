#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTHUD.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"

void AOKRTGameMode::SmokeControlsTests()
{
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* UI=CastChecked<AOKRTHUD>(PC->GetHUD());
    FVector2D Ground=FVector2D::ZeroVector,TNT=FVector2D::ZeroVector;
    FVector2D Guard=FVector2D::ZeroVector,Second=FVector2D::ZeroVector;
    const FVector Destination(720,1260,0);
    Require(PC->ProjectWorldLocationToScreen(Destination,Ground) &&
        PC->ProjectWorldLocationToScreen(TNTLocation,TNT) &&
        PC->ProjectWorldLocationToScreen(Enemies[0]->GetActorLocation(),Guard) &&
        PC->ProjectWorldLocationToScreen(Party[1]->GetActorLocation(),Second) &&
        !UI->OverUI(Ground) && !UI->OverUI(TNT) && !UI->OverUI(Guard),
        TEXT("RTT control fixture projects world targets outside HUD"));
    if (SmokeStage<0) return;
    auto Click=[PC](FVector2D Position,bool Append=false)
    { PC->BeginPointer(Position,Append); PC->EndPointer(Position); };
    auto Key=[PC,this](FKey K,EInputEvent Event)
    {
        PC->InputKey(FInputKeyParams(K,Event,Event==IE_Released ? 0.0 : 1.0));
        PC->PlayerInput->ProcessInputStack({PC->InputComponent},0,bTacticalPause);
    };
    auto Press=[&Key](FKey K) { Key(K,IE_Pressed); Key(K,IE_Released); };
    Select(0); TogglePause();
    Click(Ground);
    Require(Party[0]->QueueSize()==0 && Party[1]->QueueSize()==0,
        TEXT("left-click terrain only selects and never moves"));
    Click(Second,true);
    Require(Party[0]->IsSelected() && Party[1]->IsSelected() && ActiveMember==1,
        TEXT("Shift click selection uses modifier captured at press"));
    Select(0);
    Require(PC->CommandAt(Ground) && Party[0]->QueueSize()==1 && Party[1]->QueueSize()==0,
        TEXT("right-click terrain queues active unit movement during pause"));
    Require(PC->CommandAt(Ground,true) && Party[0]->QueueSize()==1,
        TEXT("right double-click upgrades paused move without duplicate"));
    Require(Party[0]->Stance==EOKStance::Walk && UGameplayStatics::IsGamePaused(GetWorld()),
        TEXT("double-click does not change stance or simulation while paused"));
    Require(!PC->CommandAt(Second) && ActiveMember==0 && !Party[1]->IsSelected() && Party[0]->QueueSize()==1,
        TEXT("right-click friendly does not select or replace an order"));
    PC->Arm(EOKOrder::Distract);
    Require(PC->CommandAt(Ground) && !PC->bArmed && Party[0]->QueueSize()==1,
        TEXT("right-click cancels targeting without cancelling queued movement"));
    PC->Arm(EOKOrder::Distract); Press(EKeys::Escape);
    Require(!PC->bArmed && !bMenu && Party[0]->QueueSize()==1,
        TEXT("Escape cancels ability targeting before opening menu"));
    TogglePause(); Party[0]->Tick(0);
    Require(Party[0]->Stance==EOKStance::Run && Party[0]->QueueSize()==1,
        TEXT("double-click run begins when tactical pause resumes"));
    Party[0]->CancelOrders(); Party[0]->SetStance(EOKStance::Crouch);
    Require(PC->CommandAt(Ground),TEXT("right-click accepts a crouched movement order"));
    Party[0]->Tick(0);
    Require(Party[0]->Stance==EOKStance::Crouch,TEXT("ordinary movement preserves stealth stance"));
    Party[0]->CancelOrders(); Party[0]->SetStance(EOKStance::Walk); TogglePause();
    SelectAll();
    Require(PC->CommandAt(Ground) && Party[0]->QueueSize()==1 && Party[1]->QueueSize()==1,
        TEXT("right-click issues formation movement to selected group"));
    Require(PC->CommandAt(Ground,true) && Party[0]->QueueSize()==1 && Party[1]->QueueSize()==1,
        TEXT("double-click promotes both formation destinations once"));
    Party[0]->CancelOrders(); Party[1]->CancelOrders();
    Require(PC->CommandAt(TNT) && Party[ActiveMember]->QueueSize()==1 && Party[1-ActiveMember]->QueueSize()==0,
        TEXT("context TNT interaction is assigned only to active member"));
    TArray<FVector> Route; Party[ActiveMember]->GetQueuedLocations(Route);
    Require(Route.Num()==1 && Route[0].Equals(TNTLocation,.1),TEXT("context interaction exposes approach route"));
    Require(!PC->CommandAt(TNT,true) && Party[ActiveMember]->QueueSize()==1,
        TEXT("double-click does not duplicate an interaction"));
    FVector Approach;
    Require(!bHasTNT && FindInteractionAt(DetonatorLocation,Approach)==EOKInteraction::DetonateBridge &&
        Approach.Equals(DetonatorLocation,.1),TEXT("detonator can be planned before TNT is collected"));
    Require(FindInteractionAt(HideLocation,Approach)==EOKInteraction::HideBody,
        TEXT("body hide can be planned before a queued pickup executes"));
    const FVector Saved=Party[0]->GetActorLocation();
    Party[0]->SetActorLocation(TNTLocation+FVector(0,0,90));
    Interact(Party[0],EOKInteraction::HideBody);
    Require(!bHasTNT,TEXT("typed hide interaction cannot collect nearby TNT"));
    Party[0]->SetActorLocation(Saved);
    Party[0]->CancelOrders(); Party[1]->CancelOrders(); Select(0);
    const bool bCone=Enemies[0]->Vision->bConeVisible;
    Click(Guard);
    Require(Enemies[0]->Vision->bConeVisible!=bCone && Party[0]->QueueSize()==0,
        TEXT("left-click enemy inspects vision without attacking"));
    Require(PC->CommandAt(Guard) && Party[0]->QueueSize()==1 && Party[1]->QueueSize()==0,
        TEXT("right-click living guard queues contextual takedown"));
    Party[0]->CancelOrders(); Enemies[0]->Health=0;
    Require(PC->CommandAt(Guard) && Party[0]->QueueSize()==1,
        TEXT("right-click neutralized guard queues body pickup"));
    Party[0]->CancelOrders(); Enemies[0]->Health=100; Enemies[0]->Vision->bConeVisible=bCone;
    SelectAll(); Press(EKeys::Tab);
    Require(ActiveMember==1 && Party[0]->IsSelected() && Party[1]->IsSelected(),
        TEXT("Tab cycles active specialist without dropping group selection"));
    Select(0); Key(EKeys::LeftControl,IE_Pressed); Press(EKeys::A); Key(EKeys::LeftControl,IE_Released);
    Require(Party[0]->IsSelected() && Party[1]->IsSelected(),TEXT("Ctrl A selects living party while paused"));
    Select(0); Party[1]->Health=0; Select(1); SelectAll();
    Require(ActiveMember==0 && Party[0]->IsSelected() && !Party[1]->IsSelected(),
        TEXT("hotkeys and select all exclude dead units"));
    Party[1]->Health=100;
    Require(!PC->CommandAt(FVector2D(25,30)) && Party[0]->QueueSize()==0,
        TEXT("HUD blocks right-click world commands"));
    Require(PC->TapAt(Ground) && Party[0]->QueueSize()==1,
        TEXT("touch terrain tap retains movement without mouse selection ambiguity"));
    Press(EKeys::S);
    Require(Party[0]->QueueSize()==0 && Party[1]->QueueSize()==0,
        TEXT("S stops selected units during tactical pause"));
    Require(PC->TapAt(Second) && ActiveMember==1 && Party[1]->IsSelected() && !Party[0]->IsSelected(),
        TEXT("touch party tap selects a specialist"));
    PC->Arm(EOKOrder::Distract); Click(Ground);
    Require(!PC->bArmed && Party[1]->QueueSize()==1 && Party[0]->QueueSize()==0,
        TEXT("armed left-click still targets an individual ability"));
    Party[1]->CancelOrders();
    bool bDoubleClick=false;
    for (const auto& Binding:PC->InputComponent->KeyBindings)
        bDoubleClick|=Binding.Chord.Key==EKeys::RightMouseButton && Binding.KeyEvent==IE_DoubleClick && Binding.bExecuteWhenPaused;
    Require(bDoubleClick,TEXT("right double-click binding executes during tactical pause"));
    ToggleMenu();
    Require(!PC->CommandAt(Ground) && Party[1]->QueueSize()==0,TEXT("menu gates RTT world commands"));
    ToggleMenu(); TogglePause(); Select(0); PC->CancelPointer(); PC->bArmed=false;
}
