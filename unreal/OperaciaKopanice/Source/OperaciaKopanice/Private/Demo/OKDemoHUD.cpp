#include "Demo/OKDemoHUD.h"
#include "Demo/OKDemoGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

void AOKDemoHUD::Button(int32 Command,const FString& Label,float X,float Y,float Width,bool bChecked,bool bToggle)
{
    const FBox2D Bounds(FVector2D(X,Y),FVector2D(X+Width,Y+40));
    Buttons.Add(Bounds); Commands.Add(Command);
    float MX=0,MY=0;
    const bool Hover=PlayerOwner->GetMousePosition(MX,MY) && Bounds.IsInside(FVector2D(MX,MY));
    DrawRect(Hover ? FLinearColor(.23,.25,.26,.98) : FLinearColor(.07,.08,.085,.97),X,Y,Width,40);
    DrawRect(FLinearColor(.62,.49,.25,1),X,Y+39,Width,1);
    float TextX=X+12;
    if (bToggle)
    {
        DrawRect(FLinearColor(.4,.43,.45,1),X+12,Y+11,18,18);
        DrawRect(FLinearColor(.09,.1,.11,1),X+14,Y+13,14,14);
        if (bChecked)
        {
            DrawLine(X+15,Y+20,X+19,Y+24,FLinearColor(.9,.73,.4),2);
            DrawLine(X+19,Y+24,X+27,Y+15,FLinearColor(.9,.73,.4),2);
        }
        TextX=X+42;
    }
    DrawText(Label,FLinearColor(.93,.91,.84),TextX,Y+11,nullptr,1.05f);
}
void AOKDemoHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* Game=GetWorld()->GetAuthGameMode<AOKDemoGameMode>();
    if (!Canvas || !Game) return;
    Buttons.Reset(); Commands.Reset();
    const float Width=Canvas->SizeX, Height=Canvas->SizeY;
    const auto& Level=OKDemo::GetLevel(Game->GetState().LevelIndex);
    DrawRect(FLinearColor(.025,.029,.035,.92),0,0,Width,94);
    DrawText(FString::Printf(TEXT("OPERACIA KOPANICE / %s"),Level.Name),FLinearColor(.93,.92,.87),22,14,nullptr,1.3f);
    DrawText(Game->Objective(),FLinearColor(.89,.72,.4),22,49,nullptr,1.1f);
    DrawText(FString::Printf(TEXT("TAH %d"),Game->GetState().Turn),FLinearColor::White,Width-225,22,nullptr,1.1f);
    if (Game->GetMenuPage()==0)
    {
        Button(10,TEXT("MENU"),Width-110,16,90);
        DrawText(Game->GetMessage(),FLinearColor::White,22,Height-112,nullptr,1.f);
        const TCHAR* Labels[]={TEXT("CAKAT"),TEXT("AKCIA"),TEXT("SPAT"),TEXT("RESTART")};
        const float BW=FMath::Min(130.f,(Width-390)/4);
        for (int32 I=0;I<4;++I) Button(I,Labels[I],20+I*(BW+8),Height-64,BW);
        Button(20,TEXT("<"),Width-300,Height-64,40);
        Button(21,TEXT(">"),Width-252,Height-64,40);
        Button(22,TEXT("^"),Width-204,Height-64,40);
        Button(23,TEXT("v"),Width-156,Height-64,40);
        Button(24,TEXT("+"),Width-108,Height-64,40);
        Button(25,TEXT("-"),Width-60,Height-64,40);
        auto Label=[this](FVector P,const TCHAR* Text,FLinearColor Color)
        {
            FVector2D S;
            if (PlayerOwner->ProjectWorldLocationToScreen(P,S) && S.Y>110 && S.Y<Canvas->SizeY-120)
                DrawText(Text,Color,S.X-25,S.Y-18,nullptr,1.f);
        };
        if (!Game->GetState().bTNT) Label(Game->CellLocation(Level.TNT,100),TEXT("TNT"),FLinearColor(.95,.75,.3));
        Label(Game->CellLocation(Level.Detonator,100),TEXT("ROZBUSKA"),FLinearColor(.95,.75,.3));
        Label(Game->CellLocation(Level.Exit,100),TEXT("UNIK"),FLinearColor(.3,.9,.7));
        if (Game->GetState().Outcome==OKDemo::EOutcome::Won)
            Button(15,Game->GetState().LevelIndex+1<OKDemo::LevelCount ? TEXT("DALSIA MISIA") : TEXT("VYBER MISIE"),Width-190,110,170);
        return;
    }
    DrawRect(FLinearColor(0,0,0,.62),0,94,Width,Height-94);
    const float PanelWidth=360, X=(Width-PanelWidth)*.5f, Y=FMath::Max(112.f,(Height-360)*.5f);
    const int32 Page=Game->GetMenuPage();
    DrawText(Page==1 ? TEXT("OPERACIA KOPANICE") : Page==2 ? TEXT("OPTIONS / NASTAVENIA") : TEXT("VYBER MISIE"),
        FLinearColor(.92,.75,.42),X,Y,nullptr,1.35f);
    if (Page==1)
    {
        Button(11,Game->GetState().Turn==0 ? TEXT("HRAT") : TEXT("POKRACOVAT"),X,Y+46,PanelWidth);
        Button(12,TEXT("MISIE"),X,Y+94,PanelWidth);
        Button(13,TEXT("OPTIONS"),X,Y+142,PanelWidth);
        Button(3,TEXT("RESTART MISIE"),X,Y+190,PanelWidth);
        Button(14,TEXT("UKONCIT HRU"),X,Y+238,PanelWidth);
    }
    else if (Page==2)
    {
        Button(30,TEXT("Mriezka"),X,Y+46,PanelWidth,Game->IsGridVisible(),true);
        Button(31,TEXT("Pohyb kamery"),X,Y+94,PanelWidth,Game->IsCameraEnabled(),true);
        Button(32,TEXT("Rotacia po 45 stupnoch"),X,Y+142,PanelWidth,Game->IsSteppedRotation(),true);
        const TCHAR* Quality[]={TEXT("Nizka"),TEXT("Stredna"),TEXT("Vysoka"),TEXT("Epic")};
        Button(33,FString::Printf(TEXT("Kvalita: %s"),Quality[Game->GetQuality()]),X,Y+190,PanelWidth);
        Button(34,TEXT("Obnovit kameru"),X,Y+238,PanelWidth);
        Button(10,TEXT("SPAT"),X,Y+286,PanelWidth);
    }
    else
    {
        for (int32 I=0;I<OKDemo::LevelCount;++I)
            Button(40+I,FString::Printf(TEXT("%02d  %s"),I+1,OKDemo::GetLevel(I).Name),X,Y+46+I*48,PanelWidth);
        Button(10,TEXT("SPAT"),X,Y+214,PanelWidth);
    }
}
bool AOKDemoHUD::HandleClick(float X,float Y)
{
    auto* G=GetWorld()->GetAuthGameMode<AOKDemoGameMode>();
    if (!G) return false;
    for (int32 I=0;I<Buttons.Num();++I)
    {
        if (!Buttons[I].IsInside(FVector2D(X,Y))) continue;
        const int32 C=Commands[I];
        switch(C)
        {
            case 0:G->Act(OKDemo::EAction::Wait);break;
            case 1:G->Act(OKDemo::EAction::Interact);break;
            case 2:G->Undo();break;
            case 3:G->SetMenuPage(0);G->Restart();break;
            case 10:G->ToggleMenu();break;
            case 11:G->SetMenuPage(0);break;
            case 12:G->SetMenuPage(3);break;
            case 13:G->SetMenuPage(2);break;
            case 14:UKismetSystemLibrary::QuitGame(this,PlayerOwner,EQuitPreference::Quit,false);break;
            case 15:
                if (G->GetState().LevelIndex+1<OKDemo::LevelCount) G->SelectLevel(G->GetState().LevelIndex+1);
                else G->SetMenuPage(3);
                break;
            case 20:G->Orbit(G->IsSteppedRotation() ? -45.f : -10.f,0);break;
            case 21:G->Orbit(G->IsSteppedRotation() ? 45.f : 10.f,0);break;
            case 22:G->Orbit(0,5);break;
            case 23:G->Orbit(0,-5);break;
            case 24:G->Orbit(0,0,-180);break;
            case 25:G->Orbit(0,0,180);break;
            case 30:G->ToggleGrid();break;
            case 31:G->ToggleCamera();break;
            case 32:G->ToggleRotationMode();break;
            case 33:G->CycleQuality();break;
            case 34:G->ResetCamera();break;
            default:if (C>=40 && C<40+OKDemo::LevelCount) G->SelectLevel(C-40);break;
        }
        return true;
    }
    return G->GetMenuPage()!=0 || Y<94 || (Canvas && Y>Canvas->SizeY-80);
}
