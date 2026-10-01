#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OKDemoPlayerController.generated.h"

UCLASS()
class OPERACIAKOPANICE_API AOKDemoPlayerController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
private:
    void Click();
    void ClickAt(float X, float Y);
    void TouchStart(ETouchIndex::Type Finger, FVector Position);
    void TouchEnd(ETouchIndex::Type Finger, FVector Position);
    void TouchMove(ETouchIndex::Type Finger, FVector Position);
    FVector2D TouchOrigin;
    bool bTouchTap = false;
    void North();
    void East();
    void South();
    void West();
    void Wait();
    void Interact();
    void Restart();
    void Undo();
    void Menu();
    void Grid();
    void RotateLeft();
    void RotateRight();
    void ZoomIn();
    void ZoomOut();
    void FocusPlayer();
    void ResetView();
    float DragRemainder = 0;
};
