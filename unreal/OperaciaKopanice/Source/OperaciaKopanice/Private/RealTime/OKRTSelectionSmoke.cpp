#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTHUD.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"

void AOKRTGameMode::SmokeSelectionTests()
{
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* UI=CastChecked<AOKRTHUD>(PC->GetHUD());
    FVector2D P,Q;
    Require(PC->ProjectWorldLocationToScreen(Party[0]->GetActorLocation(),P) &&
        PC->ProjectWorldLocationToScreen(Party[1]->GetActorLocation(),Q) && !UI->OverUI(P) && !UI->OverUI(Q),
        TEXT("selection fixture projects party outside HUD"));
    if (SmokeStage<0) return;
    FBox2D Bounds(ForceInit); Bounds+=P; Bounds+=Q;
    const FVector2D Margin(6,6);
    Bounds.Min-=Margin; Bounds.Max+=Margin;
    auto Drag=[PC](FVector2D Start,FVector2D End,bool Append=false)
    { PC->BeginPointer(Start,Append); PC->UpdatePointer(End); PC->EndPointer(End); };
    FBox2D Visible;
    Select(0);
    const int32 Queue0=Party[0]->QueueSize(),Queue1=Party[1]->QueueSize();
    PC->BeginPointer(Bounds.Min,false); PC->UpdatePointer(Bounds.Max);
    Require(PC->GetSelectionBounds(Visible) && Visible.Min.Equals(Bounds.Min,.1) && Visible.Max.Equals(Bounds.Max,.1),
        TEXT("drag exposes normalized selection rectangle"));
    PC->EndPointer(Bounds.Max);
    Require(Party[0]->IsSelected() && Party[1]->IsSelected() && ActiveMember==0 && !PC->GetSelectionBounds(Visible),
        TEXT("box selects both while preserving active member"));
    Select(1); Drag(Bounds.Max,Bounds.Min);
    Require(Party[0]->IsSelected() && Party[1]->IsSelected() && ActiveMember==1,TEXT("reverse drag keeps current active member"));
    Drag(P-Margin,P+Margin);
    Require(Party[0]->IsSelected() && !Party[1]->IsSelected() && ActiveMember==0,TEXT("single-unit box replaces selection"));
    Drag(Q-Margin,Q+Margin,true);
    Require(Party[0]->IsSelected() && Party[1]->IsSelected() && ActiveMember==0,TEXT("additive drag retains previous selection"));
    Require(Party[0]->QueueSize()==Queue0 && Party[1]->QueueSize()==Queue1,TEXT("selection drags never enqueue movement"));
    Require(bTacticalPause && UGameplayStatics::IsGamePaused(GetWorld()),TEXT("box selection works during true tactical pause"));

    Select(0); Party[1]->Health=0;
    Drag(Q-Margin,Q+Margin);
    Require(Party[0]->IsSelected() && !Party[1]->IsSelected(),TEXT("dead units and empty rectangles preserve selection"));
    Party[1]->Health=100;
    FVector2D Enemy;
    if (PC->ProjectWorldLocationToScreen(Enemies[0]->GetActorLocation(),Enemy))
    {
        Drag(Enemy-Margin,Enemy+Margin);
        Require(Party[0]->IsSelected() && !Party[1]->IsSelected() && !Enemies[0]->IsSelected(),TEXT("box selection excludes enemy guards"));
    }
    PC->BeginPointer(Bounds.Min,false); PC->UpdatePointer(Bounds.Max); PC->CancelPointer(); PC->EndPointer(Bounds.Max);
    Require(!PC->GetSelectionBounds(Visible) && !Party[1]->IsSelected(),TEXT("cancelled drag does not select on release"));
    PC->BeginPointer(Bounds.Min,false); ToggleMenu(); PC->EndPointer(Bounds.Max); ToggleMenu();
    Require(!PC->GetSelectionBounds(Visible) && !Party[1]->IsSelected(),TEXT("menu cancels pending world selection"));
    PC->BeginPointer(Bounds.Min,false); PC->UpdatePointer(FVector2D(-10,-10)); PC->EndPointer(Bounds.Max);
    Require(!PC->GetSelectionBounds(Visible) && !Party[1]->IsSelected(),TEXT("leaving viewport cancels pending drag"));
    PC->BeginPointer(FVector2D(25,30),false); PC->UpdatePointer(Bounds.Max); PC->EndPointer(Bounds.Max);
    Require(!PC->GetSelectionBounds(Visible) && !Party[1]->IsSelected(),TEXT("portrait press never starts a world drag"));

    // Exercise the small-motion click branch against the real collision capsule.
    Select(1); PC->BeginPointer(P,false); PC->UpdatePointer(P+FVector2D(1,1));
    Require(!PC->GetSelectionBounds(Visible),TEXT("small pointer motion stays a click"));
    PC->EndPointer(P);
    Require(Party[0]->IsSelected() && !Party[1]->IsSelected() && ActiveMember==0,TEXT("pointer release still selects a clicked unit"));
    Require(Party[0]->QueueSize()==Queue0 && Party[1]->QueueSize()==Queue1,TEXT("cancel and UI gestures preserve command queues"));
    bool Press=false,Release=false;
    for (const auto& Binding:PC->InputComponent->KeyBindings)
        if (Binding.Chord.Key==EKeys::LeftMouseButton && Binding.bExecuteWhenPaused)
        { Press|=Binding.KeyEvent==IE_Pressed; Release|=Binding.KeyEvent==IE_Released; }
    Require(Press && Release,TEXT("mouse press and release bindings execute while paused"));
    bool TouchPress=false,TouchRepeat=false,TouchRelease=false;
    for (const auto& Binding:PC->InputComponent->TouchBindings)
    {
        TouchPress|=Binding.KeyEvent==IE_Pressed && Binding.bExecuteWhenPaused;
        TouchRepeat|=Binding.KeyEvent==IE_Repeat && Binding.bExecuteWhenPaused;
        TouchRelease|=Binding.KeyEvent==IE_Released && Binding.bExecuteWhenPaused;
    }
    Require(TouchPress && TouchRepeat && TouchRelease,TEXT("touch tap and camera gesture bindings execute while paused"));
    SelectAll();
}
