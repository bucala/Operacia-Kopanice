#include "Demo/OKDemoHUD.h"
#include "Demo/OKDemoGameMode.h"
#include "Demo/OKDemoHUDLayout.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

UTexture2D* AOKDemoHUD::Texture(const TCHAR* Name)
{
    const FName Key(Name);
    if (const auto* Existing=Textures.Find(Key)) return Existing->Get();
    const FString Path=FString::Printf(TEXT("/Game/Kopanice/ReferenceHUD/T_%s.T_%s"),Name,Name);
    auto* Loaded=LoadObject<UTexture2D>(nullptr,*Path);
    Textures.Add(Key,Loaded);
    return Loaded;
}
void AOKDemoHUD::WrappedText(const FString& Text,float X,float Y,float Width,FLinearColor Color,float Scale,int32 MaxLines)
{
    TArray<FString> Words;
    Text.ParseIntoArrayWS(Words);
    FString Line;
    int32 Row=0;
    for (const FString& Word : Words)
    {
        const FString Next=Line.IsEmpty() ? Word : Line+TEXT(" ")+Word;
        float TW=0,TH=0;
        GetTextSize(Next,TW,TH,nullptr,Scale);
        if (TW>Width && !Line.IsEmpty())
        {
            if (Row==MaxLines-1) { Line+=TEXT("..."); break; }
            DrawText(Line,Color,X,Y+Row*18,nullptr,Scale);
            ++Row; Line=Word;
        }
        else Line=Next;
    }
    float TW=0,TH=0;
    GetTextSize(Line,TW,TH,nullptr,Scale);
    DrawText(Line,Color,X,Y+Row*18,nullptr,TW>Width ? Scale*Width/TW : Scale);
}
void AOKDemoHUD::Portrait(const FBox2D& B,const TCHAR* Name,const TCHAR* Label,const FVector4& UV,bool bActive)
{
    Panels.Add(B);
    const float W=B.GetSize().X;
    DrawRect(FLinearColor(.026,.03,.035,.94),B.Min.X,B.Min.Y,W,B.GetSize().Y);
    DrawRect(FLinearColor(.65,.52,.27),B.Min.X,B.Min.Y,W,3);
    DrawRect(FLinearColor(.12,.14,.16),B.Min.X+6,B.Min.Y+9,W-12,W-12);
    if (auto* Art=Texture(Name))
        DrawTexture(Art,B.Min.X+6,B.Min.Y+9,W-12,W-12,UV.X,UV.Y,UV.Z,UV.W,FLinearColor::White);
    const FLinearColor Accent=bActive ? FLinearColor(.65,.88,.76) : FLinearColor(.89,.74,.43);
    DrawRect(Accent,B.Min.X+6,B.Max.Y-6,W-12,2);
    WrappedText(Label,B.Min.X+8,B.Max.Y-27,W-16,Accent,1.05f,1);
}
void AOKDemoHUD::IconButton(int32 C,const TCHAR* Name,const TCHAR* Hint,float X,float Y,bool bActive)
{
    const FBox2D B(FVector2D(X,Y),FVector2D(X+44,Y+44));
    Buttons.Add(B); Commands.Add(C);
    float MX=0,MY=0;
    const bool Hover=PlayerOwner->GetMousePosition(MX,MY) && B.IsInside(FVector2D(MX,MY));
    DrawRect(Hover ? FLinearColor(.19,.22,.23,.97) : FLinearColor(.025,.031,.039,.94),X,Y,44,44);
    DrawRect(bActive ? FLinearColor(.65,.88,.76) : FLinearColor(.65,.52,.27),X,Y+42,44,2);
    if (auto* Icon=Texture(Name)) DrawTexture(Icon,X+10,Y+10,24,24,0,0,1,1,FLinearColor(.91,.77,.48));
    if (Hover) Tooltip=Hint;
}
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
    WrappedText(Label,TextX,Y+11,Width-(TextX-X)-10,FLinearColor(.93,.91,.84),1.05f,1);
}
void AOKDemoHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* Game=GetWorld()->GetAuthGameMode<AOKDemoGameMode>();
    if (!Canvas || !Game) return;
    Buttons.Reset(); Commands.Reset(); Panels.Reset(); Tooltip.Reset();
    const float Width=Canvas->SizeX, Height=Canvas->SizeY;
    const auto& Level=OKDemo::GetLevel(Game->GetState().LevelIndex);
    if (Game->GetMenuPage()==0)
    {
        const auto L=OKHUD::Layout(Width,Height);
        Portrait(L.Officer,TEXT("Officer"),TEXT("DOSTOJNIK"),FVector4(.385,.032,.245,.25),false);
        Portrait(L.Partisan,TEXT("Partisan"),TEXT("PARTIZAN"),FVector4(.395,.077,.23,.265),true);
        Buttons.Add(L.Partisan); Commands.Add(26);
        Panels.Add(L.Objective);
        DrawRect(FLinearColor(.025,.029,.035,.92),L.Objective.Min.X,L.Objective.Min.Y,L.Objective.GetSize().X,100);
        WrappedText(Level.Name,L.Objective.Min.X+12,L.Objective.Min.Y+10,L.Objective.GetSize().X-72,FLinearColor(.94,.93,.90),1.15f,1);
        WrappedText(Game->Objective(),L.Objective.Min.X+12,L.Objective.Min.Y+35,L.Objective.GetSize().X-24,FLinearColor(.89,.74,.43),1.1f,2);
        DrawText(FString::Printf(TEXT("TAH %d"),Game->GetState().Turn),FLinearColor(.74,.79,.81),L.Objective.Min.X+12,L.Objective.Min.Y+80,nullptr,.95f);
        IconButton(10,TEXT("menu"),TEXT("Menu"),L.Objective.Max.X-52,L.Objective.Min.Y+8);
        const TCHAR* CameraIcons[]={TEXT("rotate_ccw"),TEXT("rotate_cw"),TEXT("chevron_up"),TEXT("chevron_down"),TEXT("crosshair"),TEXT("zoom_in"),TEXT("zoom_out"),TEXT("grid_2x2")};
        const TCHAR* CameraHints[]={TEXT("Otocit vlavo"),TEXT("Otocit vpravo"),TEXT("Naklonit nahor"),TEXT("Naklonit nadol"),TEXT("Zamerat partizana"),TEXT("Priblizit"),TEXT("Oddialit"),TEXT("Mriezka")};
        const int32 CameraCommands[]={20,21,22,23,26,24,25,30};
        for (int32 I=0;I<8;++I)
            IconButton(CameraCommands[I],CameraIcons[I],CameraHints[I],L.Camera.X+(I%4)*52,L.Camera.Y+(I/4)*52,I==7 && Game->IsGridVisible());
        const float X=L.Movement.X,Y=L.Movement.Y;
        IconButton(60,TEXT("arrow_up"),TEXT("Sever"),X+52,Y);
        IconButton(63,TEXT("arrow_left"),TEXT("Zapad"),X,Y+52);
        IconButton(1,TEXT("hand"),TEXT("Interakcia"),X+52,Y+52);
        IconButton(61,TEXT("arrow_right"),TEXT("Vychod"),X+104,Y+52);
        IconButton(0,TEXT("clock_3"),TEXT("Cakat"),X,Y+104);
        IconButton(62,TEXT("arrow_down"),TEXT("Juh"),X+52,Y+104);
        IconButton(2,TEXT("undo_2"),TEXT("Vratit tah"),X+104,Y+104);
        Panels.Add(L.Message);
        DrawRect(FLinearColor(.025,.029,.035,.87),L.Message.Min.X,L.Message.Min.Y,L.Message.GetSize().X,L.Message.GetSize().Y);
        DrawRect(FLinearColor(.65,.52,.27),L.Message.Min.X,L.Message.Min.Y,3,L.Message.GetSize().Y);
        WrappedText(Game->GetMessage(),L.Message.Min.X+12,L.Message.Min.Y+12,L.Message.GetSize().X-24,FLinearColor(.92,.82,.60),1.05f,3);
        auto Label=[this](FVector P,const TCHAR* Text,FLinearColor Color)
        {
            FVector2D S;
            if (PlayerOwner->ProjectWorldLocationToScreen(P,S) && S.Y>20 && S.Y<Canvas->SizeY-20 &&
                !Panels.ContainsByPredicate([&S](const FBox2D& B){return B.IsInside(S);}))
                DrawText(Text,Color,S.X-25,S.Y-18,nullptr,1.f);
        };
        if (!Game->GetState().bTNT) Label(Game->CellLocation(Level.TNT,100),TEXT("TNT"),FLinearColor(.95,.75,.3));
        Label(Game->CellLocation(Level.Detonator,100),TEXT("ROZBUSKA"),FLinearColor(.95,.75,.3));
        Label(Game->CellLocation(Level.Exit,100),TEXT("UNIK"),FLinearColor(.3,.9,.7));
        if (Game->GetState().Outcome==OKDemo::EOutcome::Won)
            Button(15,Game->GetState().LevelIndex+1<OKDemo::LevelCount ? TEXT("DALSIA MISIA") : TEXT("VYBER MISIE"),L.Objective.Min.X,L.Objective.Max.Y+8,L.Objective.GetSize().X);
        if (!Tooltip.IsEmpty())
        {
            float MX=0,MY=0; PlayerOwner->GetMousePosition(MX,MY);
            const float TX=FMath::Clamp(MX-120,8.f,Width-248),TY=FMath::Clamp(MY-38,8.f,Height-40);
            DrawRect(FLinearColor(.025,.029,.035,.98),TX,TY,240,30);
            WrappedText(Tooltip,TX+10,TY+7,220,FLinearColor(.95,.89,.76),1.f,1);
        }
        return;
    }
    DrawRect(FLinearColor(0,0,0,.70),0,0,Width,Height);
    const float PanelWidth=FMath::Min(360.f,Width-32), X=(Width-PanelWidth)*.5f, Y=FMath::Max(16.f,(Height-360)*.5f);
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
            case 26:G->FocusPlayer();break;
            case 30:G->ToggleGrid();break;
            case 31:G->ToggleCamera();break;
            case 32:G->ToggleRotationMode();break;
            case 33:G->CycleQuality();break;
            case 34:G->ResetCamera();break;
            case 60:G->Act(OKDemo::EAction::North);break;
            case 61:G->Act(OKDemo::EAction::East);break;
            case 62:G->Act(OKDemo::EAction::South);break;
            case 63:G->Act(OKDemo::EAction::West);break;
            default:if (C>=40 && C<40+OKDemo::LevelCount) G->SelectLevel(C-40);break;
        }
        return true;
    }
    return G->GetMenuPage()!=0 || Panels.ContainsByPredicate([X,Y](const FBox2D& B){return B.IsInside(FVector2D(X,Y));});
}
