#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTPlayerController.h"
#include "Engine/Engine.h"

bool AOKRTHUD::FindButton(FName Id,FVector2D& Center) const
{
    for (const auto& B:Buttons) if (B.Id==Id) { Center=B.Bounds.GetCenter(); return true; }
    return false;
}
void AOKRTHUD::MenuAction(FName Id,FName Image,const FString& Text,FVector2D P,FVector2D Size,TFunction<void()> Action,bool Active)
{
    const FBox2D Bounds(P,P+Size);
    float X=0,Y=0;
    const bool Hover=PlayerOwner->GetMousePosition(X,Y) && Bounds.IsInsideOrOn(FVector2D(X,Y));
    DrawRect(Active ? FLinearColor(.24f,.3f,.27f,.98f) : Hover ? FLinearColor(.13f,.17f,.17f,.98f) : FLinearColor(.07f,.085f,.09f,.96f),P.X,P.Y,Size.X,Size.Y);
    if (!Image.IsNone()) Icon(Image,P.X+10,P.Y+(Size.Y-22)/2,22,FLinearColor(.88f,.79f,.54f));
    Label(Text,P.X+(Image.IsNone() ? 12 : 44),P.Y+(Size.Y-16)/2,1.f,Active ? FLinearColor(.95f,.87f,.65f) : FLinearColor(.85f,.89f,.88f));
    Buttons.Add({Bounds,MoveTemp(Action),Text,Id});
}
void AOKRTHUD::DrawMenu(AOKRTGameMode* G,float W,float H)
{
    Buttons.Reset(); Panels.Reset(); Panels.Add(FBox2D(FVector2D::ZeroVector,FVector2D(W,H)));
    DrawRect(FLinearColor(.018f,.026f,.028f,.92f),0,0,W,H);
    const bool Compact=W<800;
    const float MW=FMath::Min(920.f,W-40),MX=(W-MW)/2;
    const float Top=FMath::Max(20.f,(H-650)/2);
    const FLinearColor Gold(.91f,.8f,.52f);
    Label(TEXT("OPERACIA KOPANICE"),MX,Top,Compact ? 1.6f : 2.3f,Gold);
    Label(TEXT("TAKTICKA OPERACIA / ZIMA 1944"),MX,Top+40,.9f,FLinearColor(.57f,.68f,.67f));
    DrawLine(MX,Top+66,MX+MW,Top+66,FLinearColor(.35f,.43f,.4f),1);
    const float X=MX,Y=Top+88;
    const float Footer=FMath::Min(H-66,Top+600);
    const float RowW=Compact ? MW : 370;
    auto Page=[G](EOKMenuPage P) { G->OpenMenuPage(P); };
    auto Row=[&](FName Id,FName Image,const FString& Text,float DY,TFunction<void()> Action,bool Active=false)
    { MenuAction(Id,Image,Text,FVector2D(X,Y+DY),FVector2D(RowW,44),MoveTemp(Action),Active); };
    auto Back=[&]()
    { MenuAction(TEXT("MenuBack"),TEXT("undo_2"),TEXT("Spat"),FVector2D(X,Footer),FVector2D(120,40),[G](){ G->MenuBack(); }); };
    // The existing options flag is retained for older Blueprint/test callers.
    const EOKMenuPage Current=G->bOptions ? EOKMenuPage::Options : G->MenuPage;
    if (Current==EOKMenuPage::Pause)
    {
        const bool Ended=G->bWon || G->bLost;
        Label(G->bFrontEnd ? TEXT("Hlavne menu") : G->bWon ? TEXT("Operacia uspesna") : G->bLost ? TEXT("Tim bol odhaleny") : TEXT("Operacia pozastavena"),X,Y,1.3f,Gold);
        int32 Index=0;
        if (!G->bFrontEnd && !Ended) Row(TEXT("Resume"),TEXT("play"),TEXT("Pokracovat v misii"),42+52*Index++,[G](){ G->MenuBack(); });
        if (G->bWon && G->MissionId+1<OKMissions::Count)
            Row(TEXT("NextMission"),TEXT("move_right"),TEXT("Dalsia operacia"),42+52*Index++,[G](){ G->PreviewMission=G->MissionId+1; G->OpenMenuPage(EOKMenuPage::Briefing); });
        Row(TEXT("Missions"),TEXT("grid_2x2"),TEXT("Vyber misie"),42+52*Index++,[Page](){ Page(EOKMenuPage::Missions); });
        Row(TEXT("Options"),TEXT("settings"),TEXT("Nastavenia"),42+52*Index++,[Page](){ Page(EOKMenuPage::Options); });
        if (!G->bFrontEnd) Row(TEXT("Restart"),TEXT("rotate_ccw"),TEXT("Restart misie"),42+52*Index++,[Page](){ Page(EOKMenuPage::ConfirmRestart); });
        Row(TEXT("Quit"),TEXT("menu"),TEXT("Ukoncit hru"),42+52*Index++,[Page](){ Page(EOKMenuPage::ConfirmQuit); });
        if (!Compact)
        {
            const float RX=X+420;
            Label(G->Mission().Name,RX,Y+42,1.25f,Gold);
            Wrap(G->Mission().Briefing,RX,Y+82,MW-420,1.05f);
            Label(G->bWon ? TEXT("Vysledok: tim evakuovany") : G->Mission().Setting,RX,Y+250,1.f,FLinearColor(.63f,.77f,.7f));
        }
    }
    else if (Current==EOKMenuPage::Missions)
    {
        Label(TEXT("Operacie"),X,Y,1.35f,Gold);
        for (int32 I=0;I<OKMissions::Count;++I)
        {
            const auto& M=OKMissions::Get(I);
            const float RY=Y+44+I*(H<650 ? 78 : 102);
            MenuAction(FName(*FString::Printf(TEXT("Mission%d"),I)),TEXT("move_right"),M.Name,FVector2D(X,RY),FVector2D(MW,48),
                [G,I](){ G->PreviewMission=I; G->OpenMenuPage(EOKMenuPage::Briefing); },G->MissionId==I);
            Label(M.Setting,X+12,RY+55,.95f,FLinearColor(.65f,.75f,.72f));
            Label(G->MissionCompleted(I) ? TEXT("SPLNENA") : I==G->MissionId && !G->bFrontEnd ? TEXT("AKTUALNA") : TEXT("DOSTUPNA"),X+(H<650 ? MW-105 : 12),RY+(H<650 ? 55 : 75),.8f,Gold);
        }
        Back();
    }
    else if (Current==EOKMenuPage::Briefing)
    {
        const auto& M=OKMissions::Get(G->PreviewMission);
        Label(M.Name,X,Y,1.3f,Gold);
        Label(M.Setting,X,Y+30,1.f,FLinearColor(.65f,.75f,.72f));
        Wrap(M.Briefing,X,Y+72,MW,1.1f);
        Label(TEXT("NASADENY TIM"),X,Y+(H<650 ? 214 : 236),.9f,Gold);
        Label(TEXT("Partizan / Dostojnik"),X,Y+(H<650 ? 242 : 282),1.f);
        if (H>=650) Label(M.Id==0 ? TEXT("Kompaktna osada") : TEXT("Lesna oblast / 86.4 x 86.4 m"),X,Y+350,.95f,FLinearColor(.65f,.75f,.72f));
        Back();
        MenuAction(TEXT("Deploy"),TEXT("play"),TEXT("Nasadit tim"),FVector2D(X+MW-180,Footer),FVector2D(180,40),[G](){ G->StartMission(G->PreviewMission); },true);
    }
    else if (Current==EOKMenuPage::Options)
    {
        const TCHAR* Tabs[]={TEXT("Obraz"),TEXT("Kamera"),TEXT("Rozhranie")};
        for (int32 I=0;I<3;++I)
            MenuAction(FName(*FString::Printf(TEXT("OptionsTab%d"),I)),NAME_None,Tabs[I],FVector2D(X+I*(MW/3),Y),FVector2D(MW/3-4,40),[this,I](){ OptionsTab=I; },OptionsTab==I);
        auto Toggle=[&](FName Id,const TCHAR* Text,float DY,bool Value,TFunction<void()> Action)
        { Row(Id,Value ? TEXT("circle_dot") : TEXT("minus"),Text,DY,MoveTemp(Action),Value); };
        if (OptionsTab==0)
        {
            Label(TEXT("Kvalita renderovania"),X,Y+65,1.05f,Gold);
            const TCHAR* Names[]={TEXT("Nizka"),TEXT("Stredna"),TEXT("Vysoka"),TEXT("Ultra")};
            for (int32 I=0;I<4;++I)
                MenuAction(FName(*FString::Printf(TEXT("Quality%d"),I)),NAME_None,Names[I],FVector2D(X+I*MW/4,Y+98),FVector2D(MW/4-4,42),[G,I](){ while (G->Quality!=I) G->CycleQuality(); },G->Quality==I);
            Label(TEXT("Windows / Lumen / Nanite"),X,Y+169,.95f,FLinearColor(.65f,.75f,.72f));
        }
        if (OptionsTab==1)
        {
            Toggle(TEXT("CameraMode"),TEXT("Otacanie po 45 stupnoch"),64,G->bSteppedCamera,[G](){ G->bSteppedCamera=!G->bSteppedCamera; G->SavePreferences(); });
            Label(FString::Printf(TEXT("Citlivost kamery: %.2f"),G->CameraSensitivity),X,Y+142,1.05f,Gold);
            MenuAction(TEXT("SensitivityDown"),TEXT("minus"),TEXT(""),FVector2D(X,Y+178),FVector2D(44,44),[G](){ G->CameraSensitivity=FMath::Max(.5f,G->CameraSensitivity-.25f); G->SavePreferences(); });
            MenuAction(TEXT("SensitivityUp"),TEXT("zoom_in"),TEXT(""),FVector2D(X+56,Y+178),FVector2D(44,44),[G](){ G->CameraSensitivity=FMath::Min(2.f,G->CameraSensitivity+.25f); G->SavePreferences(); });
        }
        if (OptionsTab==2)
        {
            Toggle(TEXT("Cones"),TEXT("Zorne kuzele"),64,G->bCones,[G](){ G->ToggleCones(); });
            Toggle(TEXT("Routes"),TEXT("Trasy rozkazov"),120,G->bPathPreview,[G](){ G->bPathPreview=!G->bPathPreview; G->SavePreferences(); });
            Toggle(TEXT("Markers"),TEXT("Znacky cielov"),176,G->bObjectiveMarkers,[G](){ G->bObjectiveMarkers=!G->bObjectiveMarkers; G->SavePreferences(); });
        }
        Back();
    }
    else
    {
        const bool Restarting=Current==EOKMenuPage::ConfirmRestart;
        Label(Restarting ? TEXT("Restartovat aktualnu misiu?") : TEXT("Ukoncit hru?"),X,Y,1.3f,Gold);
        Wrap(TEXT("Rozpracovany postup tejto misie sa strati. Dokoncene operacie a nastavenia zostanu ulozene."),X,Y+54,MW,1.05f);
        Back();
        MenuAction(TEXT("Confirm"),Restarting ? TEXT("rotate_ccw") : TEXT("menu"),Restarting ? TEXT("Restartovat") : TEXT("Ukoncit"),FVector2D(X+MW-180,Footer),FVector2D(180,40),[G,Restarting,this](){ if (Restarting) G->Restart(); else PlayerOwner->ConsoleCommand(TEXT("quit")); });
    }
}
