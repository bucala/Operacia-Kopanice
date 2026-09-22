#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OKDemoHUD.generated.h"

UCLASS()
class OPERACIAKOPANICE_API AOKDemoHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool HandleClick(float X, float Y);
private:
    TArray<FBox2D> Buttons;
    TArray<int32> Commands;
    void Button(int32 Command, const FString& Label, float X, float Y, float Width, bool bChecked = false, bool bToggle = false);
};
