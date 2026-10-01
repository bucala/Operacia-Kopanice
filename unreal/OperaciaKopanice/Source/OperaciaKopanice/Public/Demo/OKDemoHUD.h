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
    TArray<FBox2D> Panels;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<class UTexture2D>> Textures;
    FString Tooltip;
    void IconButton(int32 Command, const TCHAR* Icon, const TCHAR* Hint, float X, float Y, bool bActive = false);
    void Portrait(const FBox2D& Bounds, const TCHAR* Texture, const TCHAR* Label, const FVector4& UV, bool bActive);
    void WrappedText(const FString& Text, float X, float Y, float Width, FLinearColor Color, float Scale = 1.f, int32 MaxLines = 3);
    UTexture2D* Texture(const TCHAR* Name);
    void Button(int32 Command, const FString& Label, float X, float Y, float Width, bool bChecked = false, bool bToggle = false);
};
