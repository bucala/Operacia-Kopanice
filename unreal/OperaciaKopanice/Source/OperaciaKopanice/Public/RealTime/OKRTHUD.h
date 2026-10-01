#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OKRTHUD.generated.h"

class UTexture2D;
class AOKRTGameMode;

UCLASS()
class OPERACIAKOPANICE_API AOKRTHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool Click(FVector2D Point);
    bool OverUI(FVector2D Point) const;
    bool ValidateLayout() const;
private:
    struct FButton { FBox2D Bounds; TFunction<void()> Action; FString Tip; };
    TArray<FButton> Buttons;
    TArray<FBox2D> Panels;
    FVector2D RenderSize=FVector2D::ZeroVector;
    UPROPERTY() TMap<FName,TObjectPtr<UTexture2D>> Textures;
    UTexture2D* Texture(FName Name);
    void Icon(FName Name,float X,float Y,float Size,FLinearColor Color=FLinearColor::White);
    void Button(FName IconName,float X,float Y,float Size,const FString& Tip,TFunction<void()> Action,bool Active=false);
    void Label(const FString& Text,float X,float Y,float Scale=1,FLinearColor Color=FLinearColor::White);
    void Wrap(const FString& Text,float X,float Y,float Width,float Scale=1);
    void WorldRing(FVector Location,float Radius,FLinearColor Color);
};
