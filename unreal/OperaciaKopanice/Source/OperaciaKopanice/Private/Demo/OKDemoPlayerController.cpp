#include "Demo/OKDemoPlayerController.h"
#include "Demo/OKDemoGameMode.h"
#include "Demo/OKDemoHUD.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

void AOKDemoPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    FInputModeGameAndUI Input;
    Input.SetHideCursorDuringCapture(false);
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Input);
}
void AOKDemoPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ThisClass::Click);
    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ThisClass::North);
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ThisClass::North);
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ThisClass::East);
    InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ThisClass::East);
    InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ThisClass::South);
    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ThisClass::South);
    InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ThisClass::West);
    InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ThisClass::West);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ThisClass::Wait);
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ThisClass::Interact);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ThisClass::Restart);
    InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ThisClass::Undo);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::Menu);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ThisClass::Grid);
    InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ThisClass::RotateLeft);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ThisClass::RotateRight);
    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ThisClass::ZoomIn);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ThisClass::ZoomOut);
    InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ThisClass::FocusPlayer);
    InputComponent->BindKey(EKeys::End, IE_Pressed, this, &ThisClass::ResetView);
    InputComponent->BindTouch(IE_Pressed,this,&ThisClass::TouchStart);
    InputComponent->BindTouch(IE_Released,this,&ThisClass::TouchEnd);
    InputComponent->BindTouch(IE_Repeat,this,&ThisClass::TouchMove);
}
void AOKDemoPlayerController::Click()
{
    float X, Y;
    if (!GetMousePosition(X,Y)) return;
    ClickAt(X,Y);
}
void AOKDemoPlayerController::TouchStart(ETouchIndex::Type Finger,FVector Position)
{
    bTouchTap=Finger==ETouchIndex::Touch1;
    if (bTouchTap) TouchOrigin=FVector2D(Position.X,Position.Y);
}
void AOKDemoPlayerController::TouchEnd(ETouchIndex::Type Finger,FVector Position)
{
    if (Finger==ETouchIndex::Touch1 && bTouchTap &&
        FVector2D::Distance(TouchOrigin,FVector2D(Position.X,Position.Y))<18)
        ClickAt(Position.X,Position.Y);
    bTouchTap=false;
}
void AOKDemoPlayerController::TouchMove(ETouchIndex::Type Finger,FVector Position)
{
    if (Finger!=ETouchIndex::Touch1 || FVector2D::Distance(TouchOrigin,FVector2D(Position.X,Position.Y))>=18)
        bTouchTap=false;
}
void AOKDemoPlayerController::ClickAt(float X,float Y)
{
    if (auto* HUD = Cast<AOKDemoHUD>(GetHUD()))
        if (HUD->HandleClick(X,Y)) return;
    // Intersect with the logical ground plane, so props never swallow tile clicks.
    FVector Origin, Direction;
    if (!DeprojectScreenPositionToWorld(X,Y,Origin, Direction) || FMath::IsNearlyZero(Direction.Z)) return;
    const double T = -Origin.Z / Direction.Z;
    if (T < 0) return;
    const FVector Point = Origin + Direction * T;
    if (auto* Game = GetWorld()->GetAuthGameMode<AOKDemoGameMode>())
        Game->ClickCell(FIntPoint(FMath::RoundToInt(Point.X / OKDemo::CellSize),
            FMath::RoundToInt(Point.Y / OKDemo::CellSize)));
}
#define OK_ACTION(Method, Action) void AOKDemoPlayerController::Method() { if (auto* Game = GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) Game->Act(OKDemo::EAction::Action); }
OK_ACTION(North, North)
OK_ACTION(East, East)
OK_ACTION(South, South)
OK_ACTION(West, West)
OK_ACTION(Wait, Wait)
OK_ACTION(Interact, Interact)
#undef OK_ACTION
void AOKDemoPlayerController::Restart() { if (auto* Game = GetWorld()->GetAuthGameMode<AOKDemoGameMode>(); Game && Game->GetMenuPage()==0) Game->Restart(); }
void AOKDemoPlayerController::Undo() { if (auto* Game = GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) Game->Undo(); }
void AOKDemoPlayerController::Menu() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) G->ToggleMenu(); }
void AOKDemoPlayerController::Grid() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) G->ToggleGrid(); }
void AOKDemoPlayerController::RotateLeft() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>(); G && G->IsSteppedRotation()) G->Orbit(-45,0); }
void AOKDemoPlayerController::RotateRight() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>(); G && G->IsSteppedRotation()) G->Orbit(45,0); }
void AOKDemoPlayerController::ZoomIn() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) G->Orbit(0,0,-180); }
void AOKDemoPlayerController::ZoomOut() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) G->Orbit(0,0,180); }
void AOKDemoPlayerController::FocusPlayer() { if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>()) G->FocusPlayer(); }
void AOKDemoPlayerController::ResetView()
{
    if (auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>(); G && G->GetMenuPage()==0 && G->IsCameraEnabled())
        G->ResetCamera();
}
void AOKDemoPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>();
    if (!G || G->GetMenuPage()!=0 || !G->IsCameraEnabled()) { DragRemainder=0; return; }
    FVector2D Pan(float(IsInputKeyDown(EKeys::L))-float(IsInputKeyDown(EKeys::J)),
        float(IsInputKeyDown(EKeys::I))-float(IsInputKeyDown(EKeys::K)));
    Pan=Pan.GetSafeNormal()*5.f*FMath::Min(DeltaTime,.1f);
    G->PanCamera(Pan.X,Pan.Y);
    if (!G->IsSteppedRotation())
        G->Orbit((float(IsInputKeyDown(EKeys::C))-float(IsInputKeyDown(EKeys::Q)))*65*DeltaTime,0);
    G->Orbit(0,(float(IsInputKeyDown(EKeys::PageUp))-float(IsInputKeyDown(EKeys::PageDown)))*25*DeltaTime);
    if (IsInputKeyDown(EKeys::MiddleMouseButton))
    {
        float X=0,Y=0;
        GetInputMouseDelta(X,Y);
        G->PanCamera(-X*.02f,-Y*.02f);
        DragRemainder=0;
    }
    else if (IsInputKeyDown(EKeys::RightMouseButton))
    {
        float X=0,Y=0;
        GetInputMouseDelta(X,Y);
        if (G->IsSteppedRotation())
        {
            DragRemainder+=X;
            if (FMath::Abs(DragRemainder)>=32)
            {
                const int32 Steps=FMath::TruncToInt(DragRemainder/32);
                G->Orbit(Steps*45,0);
                DragRemainder-=Steps*32;
            }
        }
        else G->Orbit(X*.3f,0);
        G->Orbit(0,-Y*.2f);
    }
    else DragRemainder=0;
}
