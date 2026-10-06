#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RealTime/OKRTTypes.h"
#include "OKRTPlayerController.generated.h"

class AOKRTGameMode;

UCLASS()
class OPERACIAKOPANICE_API AOKRTPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AOKRTPlayerController();
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float Delta) override;
    void Arm(EOKOrder Order);
    void SetPartyStance(EOKStance Stance);
    void BeginPointer(FVector2D Position,bool bAppend);
    void UpdatePointer(FVector2D Position);
    void EndPointer(FVector2D Position);
    void CancelPointer();
    bool CommandAt(FVector2D Position,bool bRun=false,bool bAppend=false);
    bool TapAt(FVector2D Position);
    bool GetSelectionBounds(FBox2D& Bounds) const;
    bool bArmed=false;
    EOKOrder ArmedOrder=EOKOrder::Move;
private:
    AOKRTGameMode* Game() const;
    void LeftClick();
    void LeftRelease();
    void RightClick();
    void RightDoubleClick();
    void Interact();
    void Pause();
    void Menu();
    void One(); void Two(); void All();
    void NextUnit();
    void Walk(); void Run(); void Crouch(); void Prone();
    void Takedown(); void Throw(); void Carry();
    void ZoomIn(); void ZoomOut(); void RotateLeft(); void RotateRight();
    void Focus(); void Restart();
    void Cancel();
    void Touch(ETouchIndex::Type Finger,FVector Location);
    void TouchMove(ETouchIndex::Type Finger,FVector Location);
    void TouchRelease(ETouchIndex::Type Finger,FVector Location);
    enum class EPointerIntent : uint8 { Selection, Command, Touch };
    bool ClickAt(FVector2D Position,EPointerIntent Intent,bool bAppend=false,bool bRun=false);
    bool IsGameplayInputAllowed() const;
    bool IsShiftDown() const;
    bool IsOnViewport(FVector2D Position) const;
    void SelectInBounds(const FBox2D& Bounds);
    bool bPointerDown=false;
    bool bBoxSelecting=false;
    bool bAppendSelection=false;
    FVector2D PointerStart=FVector2D::ZeroVector;
    FVector2D PointerEnd=FVector2D::ZeroVector;
    bool bTouchActive=false;
    ETouchIndex::Type TouchFinger=ETouchIndex::Touch1;
    FVector2D TouchStart=FVector2D::ZeroVector;
    bool TouchDown[2]={false,false};
    bool bTouchGesture=false;
    FVector2D TouchPosition[2]={FVector2D::ZeroVector,FVector2D::ZeroVector};
    FVector2D TouchPrevious[2]={FVector2D::ZeroVector,FVector2D::ZeroVector};
    int32 TouchSlot(ETouchIndex::Type Finger) const;
    void ResetTouchState();
    float PreviewClock=0;
    FVector2D LastMouse=FVector2D::ZeroVector;
    bool bWasPanning=false;
    bool bWasOrbiting=false;
};
