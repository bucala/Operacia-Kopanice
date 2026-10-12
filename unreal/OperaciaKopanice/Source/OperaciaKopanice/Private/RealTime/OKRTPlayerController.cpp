#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTHUD.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/PlayerInput.h"

AOKRTPlayerController::AOKRTPlayerController()
{
    bShowMouseCursor=true;
    bShouldPerformFullTickWhenPaused=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    DefaultMouseCursor=EMouseCursor::Default;
}
AOKRTGameMode* AOKRTPlayerController::Game() const { return GetWorld()->GetAuthGameMode<AOKRTGameMode>(); }
void AOKRTPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
#if !UE_BUILD_SHIPPING
    // Editor-hosted games inherit F5 shader complexity and F9 screenshot binds.
    PlayerInput->DebugExecBindings.RemoveAll([](const FKeyBind& Binding)
    { return Binding.Key==EKeys::F5 || Binding.Key==EKeys::F9; });
#endif
    auto Bind=[this](FKey Key,void (AOKRTPlayerController::*Function)())
    { InputComponent->BindKey(Key,IE_Pressed,this,Function).bExecuteWhenPaused=true; };
    Bind(EKeys::LeftMouseButton,&AOKRTPlayerController::LeftClick);
    InputComponent->BindKey(EKeys::LeftMouseButton,IE_DoubleClick,this,&AOKRTPlayerController::LeftDoubleClick).bExecuteWhenPaused=true;
    Bind(EKeys::RightMouseButton,&AOKRTPlayerController::RightClick);
    InputComponent->BindKey(EKeys::RightMouseButton,IE_Released,this,&AOKRTPlayerController::RightRelease).bExecuteWhenPaused=true;
    Bind(EKeys::SpaceBar,&AOKRTPlayerController::Pause);
    Bind(EKeys::P,&AOKRTPlayerController::Pause);
    Bind(EKeys::Escape,&AOKRTPlayerController::Menu);
    Bind(EKeys::One,&AOKRTPlayerController::One); Bind(EKeys::Two,&AOKRTPlayerController::Two);
    Bind(EKeys::Three,&AOKRTPlayerController::All);
    InputComponent->BindKey(FInputChord(EKeys::A,false,true,false,false),IE_Pressed,this,&AOKRTPlayerController::All).bExecuteWhenPaused=true;
    Bind(EKeys::Tab,&AOKRTPlayerController::NextUnit);
    Bind(EKeys::E,&AOKRTPlayerController::Interact);
    Bind(EKeys::W,&AOKRTPlayerController::Walk); Bind(EKeys::R,&AOKRTPlayerController::Run);
    Bind(EKeys::C,&AOKRTPlayerController::Crouch); Bind(EKeys::V,&AOKRTPlayerController::Prone);
    Bind(EKeys::T,&AOKRTPlayerController::Takedown); Bind(EKeys::F,&AOKRTPlayerController::Throw);
    Bind(EKeys::B,&AOKRTPlayerController::Carry);
    Bind(EKeys::H,&AOKRTPlayerController::Heal);
    Bind(EKeys::Z,&AOKRTPlayerController::Plan); Bind(EKeys::Enter,&AOKRTPlayerController::Execute);
    Bind(EKeys::F9,&AOKRTPlayerController::Load);
    Bind(EKeys::MouseScrollUp,&AOKRTPlayerController::ZoomIn); Bind(EKeys::MouseScrollDown,&AOKRTPlayerController::ZoomOut);
    Bind(EKeys::Q,&AOKRTPlayerController::RotateLeft); Bind(EKeys::RightBracket,&AOKRTPlayerController::RotateRight);
    Bind(EKeys::Home,&AOKRTPlayerController::Focus); Bind(EKeys::F5,&AOKRTPlayerController::Save);
    Bind(EKeys::X,&AOKRTPlayerController::Cancel);
    Bind(EKeys::S,&AOKRTPlayerController::Cancel);
    Bind(EKeys::BackSpace,&AOKRTPlayerController::UndoLastOrder);
    InputComponent->BindTouch(IE_Pressed,this,&AOKRTPlayerController::Touch).bExecuteWhenPaused=true;
    InputComponent->BindTouch(IE_Repeat,this,&AOKRTPlayerController::TouchMove).bExecuteWhenPaused=true;
    InputComponent->BindTouch(IE_Released,this,&AOKRTPlayerController::TouchRelease).bExecuteWhenPaused=true;
}
bool AOKRTPlayerController::CommandAt(FVector2D Position,bool bRun,bool bAppend)
{ return ClickAt(Position,EPointerIntent::Command,bAppend,bRun); }
bool AOKRTPlayerController::TapAt(FVector2D Position)
{ return ClickAt(Position,EPointerIntent::Touch); }
bool AOKRTPlayerController::SelectAt(FVector2D Position,bool bToggle)
{ return ClickAt(Position,EPointerIntent::Selection,bToggle); }
void AOKRTPlayerController::SelectMember(int32 Index,bool bToggle)
{
    auto* G=Game(); if (!IsGameplayInputAllowed() || !G->Party.IsValidIndex(Index)) return;
    bArmed=false;
    if (bToggle) G->ToggleSelection(Index); else G->Select(Index);
}
bool AOKRTPlayerController::ClickAt(FVector2D Position,EPointerIntent Intent,bool bAppend,bool bRun)
{
    auto* G=Game(); if (!G || !IsOnViewport(Position)) return false;
    const bool bCommand=Intent==EPointerIntent::Command;
    auto* UI=Cast<AOKRTHUD>(GetHUD());
    if (UI)
    {
        if (bCommand && UI->OverUI(Position)) return false;
        if (Intent==EPointerIntent::Touch && UI->Click(Position)) return true;
        if (Intent==EPointerIntent::Selection && UI->OverUI(Position)) return false;
    }
    if (!IsGameplayInputAllowed()) return false;
    if (Intent==EPointerIntent::Selection && bArmed)
    { bArmed=false; return true; }
    const int32 Member=UI ? UI->PartyMarkerAt(Position) : INDEX_NONE;
    if (bArmed && ArmedOrder==EOKOrder::Heal && G->Party.IsValidIndex(Member))
    {
        FOKRTOrder Order; Order.Kind=EOKOrder::Heal; Order.Target=G->Party[Member]; Order.Location=G->Party[Member]->Feet();
        if (G->Command(Order,bAppend)) { bArmed=false; return true; }
        return false;
    }
    if (G->Party.IsValidIndex(Member))
    {
        if (bCommand || bArmed) return false;
        SelectMember(Member,bAppend); return true;
    }
    FHitResult Hit;
    if (!GetHitResultAtScreenPosition(Position,ECC_Visibility,false,Hit)) return false;
    auto* Unit=Cast<AOKRTUnit>(Hit.GetActor());
    if (bArmed)
    {
        FOKRTOrder Order; Order.Kind=ArmedOrder; Order.Location=Hit.ImpactPoint; Order.Target=Unit;
        if (ArmedOrder==EOKOrder::Interact)
        {
            Order.Interaction=G->FindInteractionAt(Hit.ImpactPoint,Order.Location);
            Order.bApproachInteraction=Order.Interaction!=EOKInteraction::Nearby;
            if (!Order.bApproachInteraction) { G->Message=TEXT("Vyber interaktivny objekt."); return false; }
        }
        if (Order.Kind==EOKOrder::Distract || Order.bApproachInteraction || Unit)
        { if (G->Command(Order,bAppend)) { bArmed=false; return true; } }
        else G->Message=TEXT("Vyber ciel schopnosti.");
        return false;
    }
    if (Unit)
    {
        if (!bCommand)
        {
            if (!Unit->bEnemy && Unit->IsAlive()) SelectMember(G->Party.IndexOfByKey(Unit),bAppend);
            else if (Unit->bEnemy && Unit->IsAlive()) Unit->Vision->bConeVisible=!Unit->Vision->bConeVisible;
            return true;
        }
        if (!Unit->bEnemy || bRun) return false;
        if (Unit->IsAlive()) { G->Message=TEXT("Najprv vyber schopnost ticheho utoku."); return false; }
        FOKRTOrder Order; Order.Kind=EOKOrder::Carry;
        Order.Target=Unit; Order.Location=Unit->Feet();
        return G->Command(Order,bAppend);
    }
    if (Intent==EPointerIntent::Selection) return false;
    FOKRTOrder Order; Order.Location=Hit.ImpactPoint;
    FVector Interaction;
    Order.Interaction=G->FindInteractionAt(Hit.ImpactPoint,Interaction);
    if (Order.Interaction!=EOKInteraction::Nearby)
    {
        if (bRun) return false;
        Order.Kind=EOKOrder::Interact; Order.Location=Interaction; Order.bApproachInteraction=true;
    }
    else Order.bRunToDestination=bRun;
    return G->Command(Order,bAppend);
}
bool AOKRTPlayerController::IsGameplayInputAllowed() const
{ const auto* G=Game(); return G && !G->bMenu && !G->bWon && !G->bLost; }
bool AOKRTPlayerController::IsShiftDown() const
{ return IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift); }
bool AOKRTPlayerController::IsControlDown() const
{ return IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl); }
bool AOKRTPlayerController::IsOnViewport(FVector2D Position) const
{
    int32 W,H; GetViewportSize(W,H);
    return !Position.ContainsNaN() && Position.X>=0 && Position.Y>=0 && Position.X<W && Position.Y<H;
}
void AOKRTPlayerController::CancelPointer() { bPointerDown=false; bBoxSelecting=false; }
void AOKRTPlayerController::BeginPointer(FVector2D Position,bool bAppend)
{
    CancelPointer();
    if (!IsOnViewport(Position)) return;
    if (!IsGameplayInputAllowed()) return;
    // RMB cancels a tool; a separate gesture is required to select afterwards.
    if (bArmed) { bArmed=false; return; }
    if (auto* UI=Cast<AOKRTHUD>(GetHUD())) if (UI->OverUI(Position)) return;
    bPointerDown=true; bAppendSelection=bAppend;
    PointerStart=PointerEnd=Position;
}
void AOKRTPlayerController::UpdatePointer(FVector2D Position)
{
    if (!bPointerDown) return;
    if (!IsGameplayInputAllowed() || !IsOnViewport(Position)) { CancelPointer(); return; }
    PointerEnd=Position;
    if (FVector2D::Distance(PointerStart,PointerEnd)>=8) bBoxSelecting=true;
}
bool AOKRTPlayerController::GetSelectionBounds(FBox2D& Bounds) const
{
    if (!bPointerDown || !bBoxSelecting || !IsGameplayInputAllowed()) return false;
    Bounds=FBox2D(ForceInit); Bounds+=PointerStart; Bounds+=PointerEnd;
    return true;
}
void AOKRTPlayerController::SelectInBounds(const FBox2D& Bounds)
{
    auto* G=Game(); if (!G) return;
    const auto* UI=Cast<AOKRTHUD>(GetHUD());
    TArray<int32> Hits;
    for (int32 I=0;I<G->Party.Num();++I)
    {
        const auto* Unit=G->Party[I].Get();
        FVector2D Screen;
        if (Unit && Unit->IsAlive() && !Unit->IsHidden() &&
            ProjectWorldLocationToScreen(Unit->GetActorLocation(),Screen) && IsOnViewport(Screen) &&
            Bounds.IsInsideOrOn(Screen) && (!UI || !UI->OverUI(Screen))) Hits.Add(I);
    }
    // Empty drags preserve selection and never turn into movement commands.
    if (Hits.IsEmpty()) return;
    const int32 PreviousActive=G->ActiveMember;
    G->Select(Hits[0],bAppendSelection);
    for (int32 I=1;I<Hits.Num();++I) G->Select(Hits[I],true);
    if (G->Party.IsValidIndex(PreviousActive) && G->Party[PreviousActive]->IsSelected()) G->Select(PreviousActive,true);
}
void AOKRTPlayerController::EndPointer(FVector2D Position)
{
    if (!bPointerDown) return;
    UpdatePointer(Position);
    if (!bPointerDown) return;
    FBox2D Bounds;
    if (GetSelectionBounds(Bounds)) SelectInBounds(Bounds);
    else
    {
        const auto* UI=Cast<AOKRTHUD>(GetHUD());
        if (!UI || !UI->OverUI(Position)) ClickAt(Position,EPointerIntent::Selection,bAppendSelection);
    }
    CancelPointer();
}
void AOKRTPlayerController::LeftClick()
{
    // UE synthesizes IE_Pressed for IE_DoubleClick; dispatch that click only once.
    const auto* State=PlayerInput ? PlayerInput->GetKeyState(EKeys::LeftMouseButton) : nullptr;
    if (State && !State->EventCounts[IE_DoubleClick].IsEmpty()) return;
    PrimaryClick(false);
}
void AOKRTPlayerController::LeftDoubleClick() { PrimaryClick(true); }
void AOKRTPlayerController::PrimaryClick(bool bDouble)
{
    CancelPointer(); float X,Y;
    if (!GetMousePosition(X,Y)) return;
    const FVector2D Position(X,Y);
    if (auto* UI=Cast<AOKRTHUD>(GetHUD())) if (UI->Click(Position)) { bPrimaryClickValid=false; return; }
    if (!IsGameplayInputAllowed()) { bPrimaryClickValid=false; return; }
    const bool SameTarget=bDouble && bPrimaryClickValid && FVector2D::Distance(Position,LastPrimaryClick)<8;
    // Double-click run only upgrades a terrain move, not a skill/interaction.
    if (SameTarget && !bArmed && !bPrimaryClickMoved) return;
    const bool WasArmed=bArmed;
    const bool Accepted=CommandAt(Position,SameTarget && bPrimaryClickMoved && !WasArmed,IsShiftDown());
    bPrimaryClickMoved=Accepted && !WasArmed && Game()->CommandFeedback.Kind==EOKOrder::Move;
    bPrimaryClickValid=true; LastPrimaryClick=Position;
}
void AOKRTPlayerController::RightClick()
{
    CancelPointer(); float X,Y;
    if (!IsInputKeyDown(EKeys::LeftAlt) && !IsInputKeyDown(EKeys::RightAlt) && GetMousePosition(X,Y))
        BeginPointer(FVector2D(X,Y),IsControlDown());
}
void AOKRTPlayerController::RightRelease()
{ float X,Y; if (GetMousePosition(X,Y)) EndPointer(FVector2D(X,Y)); else CancelPointer(); }
int32 AOKRTPlayerController::TouchSlot(ETouchIndex::Type Finger) const
{
    if (Finger==ETouchIndex::Touch1) return 0;
    if (Finger==ETouchIndex::Touch2) return 1;
    return INDEX_NONE;
}
void AOKRTPlayerController::ResetTouchState()
{
    bTouchActive=false; bTouchGesture=false; TouchDown[0]=TouchDown[1]=false;
}
void AOKRTPlayerController::Touch(ETouchIndex::Type Finger,FVector Location)
{
    const int32 Slot=TouchSlot(Finger);
    if (Slot==INDEX_NONE || TouchDown[Slot]) return;
    const FVector2D Position(Location.X,Location.Y);
    TouchDown[Slot]=true; TouchPosition[Slot]=TouchPrevious[Slot]=Position;
    if (Slot==0)
    { bTouchActive=true; TouchFinger=Finger; TouchStart=Position; }
    if (TouchDown[0] && TouchDown[1]) bTouchGesture=true;
}
void AOKRTPlayerController::TouchMove(ETouchIndex::Type Finger,FVector Location)
{
    const int32 Slot=TouchSlot(Finger);
    if (Slot==INDEX_NONE || !TouchDown[Slot]) return;
    const FVector2D Position(Location.X,Location.Y);
    TouchPrevious[Slot]=TouchPosition[Slot]; TouchPosition[Slot]=Position;
    if (FVector2D::Distance(TouchStart,Position)>=24.f) bTouchGesture=true;
    if (!TouchDown[0] || !TouchDown[1]) return;
    if (!IsGameplayInputAllowed()) return;
    const FVector2D PreviousCenter=(TouchPrevious[0]+TouchPrevious[1])*.5f;
    const FVector2D Center=(TouchPosition[0]+TouchPosition[1])*.5f;
    const float PreviousDistance=FVector2D::Distance(TouchPrevious[0],TouchPrevious[1]);
    const float Distance=FVector2D::Distance(TouchPosition[0],TouchPosition[1]);
    if (auto* G=Game())
    {
        G->Pan((PreviousCenter.X-Center.X)*2.2f,(Center.Y-PreviousCenter.Y)*2.2f);
        if (FMath::Abs(Distance-PreviousDistance)>0.1f) G->Orbit(0,0,(PreviousDistance-Distance)*2.5f);
    }
}
void AOKRTPlayerController::TouchRelease(ETouchIndex::Type Finger,FVector Location)
{
    const int32 Slot=TouchSlot(Finger);
    if (Slot==INDEX_NONE || !TouchDown[Slot]) return;
    const FVector2D End(Location.X,Location.Y);
    const bool bTap=Slot==0 && TouchDown[1]==false && !bTouchGesture &&
        IsOnViewport(End) && FVector2D::Distance(TouchStart,End)<24.f;
    TouchDown[Slot]=false;
    if (bTap) TapAt(End);
    if (!TouchDown[0] && !TouchDown[1]) ResetTouchState();
}
void AOKRTPlayerController::Arm(EOKOrder Order)
{
    auto* G=Game();
    if (!G || G->bMenu || G->bWon || G->bLost || !G->Party.IsValidIndex(G->ActiveMember)) return;
    CancelPointer();
    bArmed=true; ArmedOrder=Order;
    if (Order==EOKOrder::Carry)
    {
        if (G->Party[G->ActiveMember]->CarriedBody.IsValid())
        { FOKRTOrder Command; Command.Kind=Order; G->Command(Command); bArmed=false; }
    }
}
void AOKRTPlayerController::SetPartyStance(EOKStance Stance)
{ if (auto* G=Game()) { FOKRTOrder Order; Order.Kind=EOKOrder::Stance; Order.Stance=Stance; G->Command(Order); } }
void AOKRTPlayerController::Interact() { Arm(EOKOrder::Interact); }
void AOKRTPlayerController::Pause() { if (auto* G=Game()) G->TogglePause(); }
void AOKRTPlayerController::Menu()
{
    const bool bCancelTarget=bArmed || bPointerDown;
    CancelPointer(); ResetTouchState(); bArmed=false;
    if (!bCancelTarget) if (auto* G=Game()) { if (G->bCommandMode) G->CancelPlan(); else G->ToggleMenu(); }
}
void AOKRTPlayerController::One() { SelectMember(0,IsControlDown()); }
void AOKRTPlayerController::Two() { SelectMember(1,IsControlDown()); }
void AOKRTPlayerController::All() { if (auto* G=Game()) G->SelectAll(); }
void AOKRTPlayerController::NextUnit()
{
    auto* G=Game(); if (!G || !IsGameplayInputAllowed()) return;
    TArray<int32> Selected;
    for (int32 I=0;I<G->Party.Num();++I)
        if (G->Party[I]->IsAlive() && G->Party[I]->IsSelected()) Selected.Add(I);
    const int32 Current=Selected.IndexOfByKey(G->ActiveMember);
    if (Selected.Num()>1) { G->Select(Selected[(Current+1)%Selected.Num()],true); return; }
    for (int32 Offset=1;Offset<=G->Party.Num();++Offset)
    {
        const int32 Index=(G->ActiveMember+Offset)%G->Party.Num();
        if (G->Party[Index]->IsAlive()) { G->Select(Index); return; }
    }
}
void AOKRTPlayerController::Walk() { SetPartyStance(EOKStance::Walk); }
void AOKRTPlayerController::Run() { SetPartyStance(EOKStance::Run); }
void AOKRTPlayerController::Crouch() { SetPartyStance(EOKStance::Crouch); }
void AOKRTPlayerController::Prone() { SetPartyStance(EOKStance::Prone); }
void AOKRTPlayerController::Takedown() { Arm(EOKOrder::Takedown); }
void AOKRTPlayerController::Throw() { Arm(EOKOrder::Distract); }
void AOKRTPlayerController::Carry() { Arm(EOKOrder::Carry); }
void AOKRTPlayerController::Heal() { Arm(EOKOrder::Heal); }
void AOKRTPlayerController::Plan() { if (IsGameplayInputAllowed()) { bArmed=false; Game()->ToggleCommandMode(); } }
void AOKRTPlayerController::Execute() { if (IsGameplayInputAllowed()) { bArmed=false; Game()->ExecutePlan(); } }
void AOKRTPlayerController::Save() { if (auto* G=Game()) G->QuickSave(); }
void AOKRTPlayerController::Load() { if (auto* G=Game()) G->QuickLoad(); }
void AOKRTPlayerController::ZoomIn() { if (auto* G=Game()) G->Orbit(0,0,-250); }
void AOKRTPlayerController::ZoomOut() { if (auto* G=Game()) G->Orbit(0,0,250); }
void AOKRTPlayerController::RotateLeft() { if (auto* G=Game()) G->Orbit(G->bSteppedCamera ? -45 : -8,0); }
void AOKRTPlayerController::RotateRight() { if (auto* G=Game()) G->Orbit(G->bSteppedCamera ? 45 : 8,0); }
void AOKRTPlayerController::Focus() { if (auto* G=Game()) G->FocusSelected(); }
void AOKRTPlayerController::Restart() { if (auto* G=Game()) G->OpenMenuPage(EOKMenuPage::ConfirmRestart); }
void AOKRTPlayerController::Cancel()
{
    CancelPointer(); ResetTouchState(); bArmed=false;
    if (!IsGameplayInputAllowed()) return;
    if (Game()->bCommandMode) { Game()->CancelPlan(); return; }
    if (auto* G=Game()) for (AOKRTUnit* U:G->Party) if (U->IsSelected()) U->CancelOrders();
}
void AOKRTPlayerController::UndoLastOrder()
{
    auto* G=Game();
    if (!IsGameplayInputAllowed() || !G->Party.IsValidIndex(G->ActiveMember)) return;
    auto* Unit=G->Party[G->ActiveMember].Get();
    if (!Unit->IsSelected() || !Unit->UndoLastOrder()) return;
    CancelPointer(); bArmed=false;
    G->Message=Unit->DisplayName+TEXT(": posledny cakajuci rozkaz zruseny.");
}
void AOKRTPlayerController::PlayerTick(float Delta)
{
    Super::PlayerTick(Delta);
    auto* G=Game(); if (!G) return;
    const float CameraDelta=FMath::Clamp(IsPaused() ? float(FApp::GetDeltaTime()) : Delta,0.f,.05f);
    if (IsGameplayInputAllowed())
    {
        const float Horizontal=float(IsInputKeyDown(EKeys::Right))-float(IsInputKeyDown(EKeys::Left));
        const float Vertical=float(IsInputKeyDown(EKeys::Up))-float(IsInputKeyDown(EKeys::Down));
        if (Horizontal || Vertical) G->Pan(Horizontal*1000*CameraDelta*G->CameraSensitivity,Vertical*1000*CameraDelta*G->CameraSensitivity);
    }
    float X=0,Y=0;
    if (!GetMousePosition(X,Y))
    { CancelPointer(); bWasPanning=bWasOrbiting=false; G->PreviewSpline->ClearSplinePoints(); return; }
    UpdatePointer(FVector2D(X,Y));
    const bool bPanning=IsGameplayInputAllowed() && IsInputKeyDown(EKeys::MiddleMouseButton);
    const bool bOrbiting=IsGameplayInputAllowed() && !G->bSteppedCamera &&
        (IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt)) && IsInputKeyDown(EKeys::RightMouseButton);
    if (bPanning && bWasPanning) G->Pan((LastMouse.X-X)*3*G->CameraSensitivity,(Y-LastMouse.Y)*3*G->CameraSensitivity);
    if (bOrbiting && bWasOrbiting)
        G->Orbit((X-LastMouse.X)*.25f*G->CameraSensitivity,(Y-LastMouse.Y)*.2f*G->CameraSensitivity);
    bWasPanning=bPanning; bWasOrbiting=bOrbiting;
    LastMouse=FVector2D(X,Y);
    PreviewClock+=IsPaused() ? FApp::GetDeltaTime() : Delta;
    if (PreviewClock>.15f)
    {
        PreviewClock=0;
        auto* UI=Cast<AOKRTHUD>(GetHUD());
        FHitResult Hit;
        CurrentMouseCursor=EMouseCursor::Default;
        if (IsGameplayInputAllowed() && !bBoxSelecting && UI && UI->PartyMarkerAt(LastMouse)!=INDEX_NONE)
        {
            CurrentMouseCursor=bArmed ? EMouseCursor::Default : EMouseCursor::Hand;
            G->PreviewSpline->ClearSplinePoints();
        }
        else if (IsGameplayInputAllowed() && !bBoxSelecting && (!UI || !UI->OverUI(LastMouse)) &&
            GetHitResultAtScreenPosition(LastMouse,ECC_Visibility,false,Hit))
        {
            auto* Unit=Cast<AOKRTUnit>(Hit.GetActor());
            FVector Destination=Hit.ImpactPoint;
            const EOKInteraction Interaction=Unit ? EOKInteraction::Nearby : G->FindInteractionAt(Destination,Destination);
            const bool bInteraction=Interaction!=EOKInteraction::Nearby;
            CurrentMouseCursor=bInteraction ? EMouseCursor::Hand : Unit && !Unit->IsAlive() ? EMouseCursor::GrabHand : EMouseCursor::Crosshairs;
            if ((Unit && ((!Unit->bEnemy && !(bArmed && ArmedOrder==EOKOrder::Heal)) || (Unit->IsAlive() && !bArmed))) ||
                (bArmed && (ArmedOrder==EOKOrder::Distract || (!Unit && ArmedOrder!=EOKOrder::Interact))) ||
                (bArmed && ArmedOrder==EOKOrder::Interact && !bInteraction))
                G->PreviewSpline->ClearSplinePoints();
            else
            {
                FOKRTOrder Order; Order.Location=Destination;
                Order.Kind=bArmed ? ArmedOrder : Unit ? EOKOrder::Carry : bInteraction ? EOKOrder::Interact : EOKOrder::Move;
                Order.Target=Unit; Order.Interaction=Interaction; Order.bApproachInteraction=bInteraction;
                G->PreviewOrder(Order,IsShiftDown());
            }
        }
        else G->PreviewSpline->ClearSplinePoints();
    }
}
