#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OKRTHUD.generated.h"

class UTexture2D;
class AOKRTGameMode;
class AOKRTUnit;

UCLASS()
class OPERACIAKOPANICE_API AOKRTHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool Click(FVector2D Point);
    bool OverUI(FVector2D Point) const;
    bool ValidateLayout() const;
    bool FindButton(FName Id,FVector2D& Center) const;
    bool FindPartyMarker(int32 Member,FVector2D& Center) const;
    int32 PartyMarkerAt(FVector2D Point) const;
    bool FindCommandFeedback(FVector2D& Center) const;
private:
    struct FPartyMarker { int32 Member; FBox2D Bounds; FVector2D Anchor; };
    TArray<FPartyMarker> BuildPartyMarkers() const;
    void DrawPartyMarkers();
    void DrawCommandFeedback();
    struct FButton { FBox2D Bounds; TFunction<void()> Action; FString Tip; FName Id=NAME_None; bool bEnabled=true; };
    TArray<FButton> Buttons;
    TArray<FBox2D> Panels;
    FVector2D RenderSize=FVector2D::ZeroVector;
    UPROPERTY() TMap<FName,TObjectPtr<UTexture2D>> Textures;
    UTexture2D* Texture(FName Name);
    void Icon(FName Name,float X,float Y,float Size,FLinearColor Color=FLinearColor::White);
    void Button(FName IconName,float X,float Y,float Size,const FString& Tip,TFunction<void()> Action,bool Active=false,bool Enabled=true);
    void Label(const FString& Text,float X,float Y,float Scale=1,FLinearColor Color=FLinearColor::White);
    void Wrap(const FString& Text,float X,float Y,float Width,float Scale=1);
    void WorldRing(FVector Location,float Radius,FLinearColor Color);
    void DrawMenu(AOKRTGameMode* Game,float Width,float Height);
    void MenuAction(FName Id,FName IconName,const FString& Text,FVector2D Position,FVector2D Size,TFunction<void()> Action,bool Active=false);
    int32 OptionsTab=0;
    void DrawOrderQueue(AOKRTGameMode* Game,FVector2D Position,float Width);
    TWeakObjectPtr<AOKRTUnit> QueueUnit;
    int32 QueuePage=0;
};
