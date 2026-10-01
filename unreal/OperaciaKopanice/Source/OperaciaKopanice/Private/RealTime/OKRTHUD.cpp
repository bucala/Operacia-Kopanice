#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Scalability.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

UTexture2D* AOKRTHUD::Texture(FName Name)
{
    if (auto* Existing=Textures.Find(Name)) return *Existing;
    const FString Path=FString::Printf(TEXT("/Game/Kopanice/ReferenceHUD/T_%s.T_%s"),*Name.ToString(),*Name.ToString());
    auto* Asset=LoadObject<UTexture2D>(nullptr,*Path); Textures.Add(Name,Asset); return Asset;
}
void AOKRTHUD::Label(const FString& Text,float X,float Y,float Scale,FLinearColor Color)
{ DrawText(Text,Color,X,Y,GEngine->GetSmallFont(),Scale); }
void AOKRTHUD::Wrap(const FString& Text,float X,float Y,float Width,float Scale)
{
    TArray<FString> Words; Text.ParseIntoArray(Words,TEXT(" "),true);
    FString Line;
    for (const auto& Word:Words)
    {
        float W,H; GetTextSize(Line+TEXT(" ")+Word,W,H,GEngine->GetSmallFont(),Scale);
        if (W>Width && !Line.IsEmpty()) { Label(Line,X,Y,Scale); Y+=20*Scale; Line=Word; }
        else Line+=(Line.IsEmpty() ? TEXT("") : TEXT(" "))+Word;
    }
    Label(Line,X,Y,Scale);
}
void AOKRTHUD::Icon(FName Name,float X,float Y,float Size,FLinearColor Color)
{
    if (auto* T=Texture(Name)) DrawTexture(T,X,Y,Size,Size,0,0,1,1,Color,BLEND_Translucent);
}
void AOKRTHUD::Button(FName Name,float X,float Y,float Size,const FString& Tip,TFunction<void()> Action,bool Active)
{
    DrawRect(Active ? FLinearColor(.35f,.31f,.18f,.95f) : FLinearColor(.035f,.04f,.045f,.88f),X,Y,Size,Size);
    Icon(Name,X+8,Y+8,Size-16,FLinearColor(.91f,.8f,.52f));
    Buttons.Add({FBox2D(FVector2D(X,Y),FVector2D(X+Size,Y+Size)),MoveTemp(Action),Tip});
}
bool AOKRTHUD::OverUI(FVector2D Point) const
{
    for (const auto& P:Panels) if (P.IsInsideOrOn(Point)) return true;
    for (const auto& B:Buttons) if (B.Bounds.IsInsideOrOn(Point)) return true;
    return false;
}
bool AOKRTHUD::Click(FVector2D Point)
{
    // Copy the callback: restarting can invalidate HUD state on the following frame.
    for (const auto& B:Buttons) if (B.Bounds.IsInsideOrOn(Point)) { auto Action=B.Action; Action(); return true; }
    return OverUI(Point);
}
bool AOKRTHUD::ValidateLayout() const
{
    if (RenderSize.IsNearlyZero() || Buttons.IsEmpty()) return false;
    for (int32 I=0;I<Buttons.Num();++I)
    {
        const auto& A=Buttons[I].Bounds;
        if (A.Min.X<0 || A.Min.Y<0 || A.Max.X>RenderSize.X || A.Max.Y>RenderSize.Y) return false;
        for (int32 J=I+1;J<Buttons.Num();++J)
        {
            const auto& B=Buttons[J].Bounds;
            if (FMath::Min(A.Max.X,B.Max.X)>FMath::Max(A.Min.X,B.Min.X) &&
                FMath::Min(A.Max.Y,B.Max.Y)>FMath::Max(A.Min.Y,B.Min.Y)) return false;
        }
    }
    return true;
}
void AOKRTHUD::WorldRing(FVector Location,float Radius,FLinearColor Color)
{
    for (int32 I=0;I<40;++I)
    {
        const float A=2*PI*I/40,B=2*PI*(I+1)/40;
        FVector2D P,Q;
        if (PlayerOwner->ProjectWorldLocationToScreen(Location+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,12),P) &&
            PlayerOwner->ProjectWorldLocationToScreen(Location+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,12),Q))
            DrawLine(P.X,P.Y,Q.X,Q.Y,Color,1);
    }
}
void AOKRTHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* G=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    auto* PC=Cast<AOKRTPlayerController>(PlayerOwner);
    if (!Canvas || !G || !PC || G->Party.Num()!=2) return;
    Buttons.Reset(); Panels.Reset();
    const float W=Canvas->SizeX,H=Canvas->SizeY;
    RenderSize=FVector2D(W,H);
    const bool Compact=W<800;
    const float Card=Compact ? 88 : 132;
    const float S=Compact ? 38 : 44;
    FBox2D Selection;
    if (PC->GetSelectionBounds(Selection))
    {
        const FVector2D A=Selection.Min,B=Selection.Max;
        const FLinearColor Edge(.85f,.93f,.86f,.9f);
        DrawRect(FLinearColor(.55f,.8f,.65f,.09f),A.X,A.Y,B.X-A.X,B.Y-A.Y);
        DrawLine(A.X,A.Y,B.X,A.Y,Edge,1); DrawLine(B.X,A.Y,B.X,B.Y,Edge,1);
        DrawLine(B.X,B.Y,A.X,B.Y,Edge,1); DrawLine(A.X,B.Y,A.X,A.Y,Edge,1);
    }
    for (int32 I=0;I<2;++I)
    {
        auto* Unit=G->Party[I].Get();
        const float X=16,Y=18+I*(Card+58);
        DrawRect(FLinearColor(.035f,.04f,.045f,.92f),X,Y,Card,Card+48);
        const bool Active=Unit->IsSelected() && G->ActiveMember==I;
        DrawRect(Active ? FLinearColor(.81f,.68f,.36f) : Unit->IsSelected() ? FLinearColor(.45f,.72f,.55f) : FLinearColor(.22f,.23f,.22f),X,Y,Card,3);
        if (auto* T=Texture(I==0 ? TEXT("Partisan") : TEXT("Officer")))
            DrawTexture(T,X+7,Y+8,Card-14,Card-14,I==0 ? .395f : .385f,I==0 ? .077f : .032f,I==0 ? .23f : .245f,I==0 ? .265f : .25f,FLinearColor::White,BLEND_Translucent);
        Label(Unit->DisplayName,X+7,Y+Card,Compact ? 1 : 1.15f,FLinearColor(.91f,.8f,.52f));
        DrawRect(FLinearColor(.15f,.15f,.15f),X+7,Y+Card+23,Card-14,4);
        DrawRect(FLinearColor(.35f,.66f,.45f),X+7,Y+Card+23,(Card-14)*Unit->Health/100,4);
        Label(FString::Printf(TEXT("%d | %d"),Unit->Distractions,Unit->QueueSize()),X+7,Y+Card+29,.85f);
        if (Active) Icon(TEXT("crosshair"),X+Card-23,Y+Card+29,16,FLinearColor(.91f,.8f,.52f));
        const FBox2D Bounds(FVector2D(X,Y),FVector2D(X+Card,Y+Card+48));
        Panels.Add(Bounds); Buttons.Add({Bounds,[G,PC,I](){ G->Select(I,PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)); },
            Unit->DisplayName+(Active ? TEXT(" (aktivna)") : TEXT(""))});
        if (Unit->IsSelected()) WorldRing(Unit->Feet(),42,Active ? FLinearColor(.91f,.8f,.52f,.9f) : FLinearColor(.65f,.9f,.73f,.7f));
    }
    const float ObjectiveWidth=FMath::Min(320.f,W-Card-64);
    DrawRect(FLinearColor(.025f,.03f,.035f,.83f),W-ObjectiveWidth-16,18,ObjectiveWidth,86);
    Panels.Add(FBox2D(FVector2D(W-ObjectiveWidth-16,18),FVector2D(W-16,104)));
    Wrap(G->Objective(),W-ObjectiveWidth-6,28,ObjectiveWidth-20,Compact ? .95f : 1.2f);
    if (G->bTacticalPause) Label(TEXT("PAUZA"),W-ObjectiveWidth-6,80,1,FLinearColor(.91f,.8f,.52f));
    for (AOKRTUnit* Enemy:G->Enemies)
    {
        if (!Enemy->IsAlive()) continue;
        FVector2D P;
        if (PC->ProjectWorldLocationToScreen(Enemy->Feet()+FVector(0,0,215),P))
        {
            auto* AI=CastChecked<AOKRTGuardController>(Enemy->GetController());
            if (AI->Suspicion>.01f)
            {
                DrawRect(FLinearColor(.07f,.07f,.07f,.85f),P.X-20,P.Y,40,5);
                DrawRect(AI->AlertState==EOKAlert::Combat ? FLinearColor(.95f,.16f,.1f) : FLinearColor(.95f,.72f,.25f),P.X-20,P.Y,40*AI->Suspicion,5);
            }
        }
    }
    for (const auto& Pulse:G->NoisePulses)
        WorldRing(Pulse.Location,Pulse.Radius*FMath::Min(1.f,Pulse.Age/.8f),FLinearColor(.9f,.8f,.35f,1-Pulse.Age/1.5f));
    for (int32 I=1;I<G->PreviewSpline->GetNumberOfSplinePoints();++I)
    {
        FVector2D A,B;
        if (PC->ProjectWorldLocationToScreen(G->PreviewSpline->GetLocationAtSplinePoint(I-1,ESplineCoordinateSpace::World),A) &&
            PC->ProjectWorldLocationToScreen(G->PreviewSpline->GetLocationAtSplinePoint(I,ESplineCoordinateSpace::World),B))
            DrawLine(A.X,A.Y,B.X,B.Y,FLinearColor(.8f,.91f,.83f,.8f),1.4f);
    }
    if (G->bPathPreview)
    {
        for (AOKRTUnit* Planned:G->Party)
        {
            if (!Planned->IsSelected() || Planned->QueueSize()==0) continue;
            TArray<FVector> Destinations; Planned->GetQueuedLocations(Destinations);
            FVector From=Planned->Feet();
            const FLinearColor PathColor=G->ActiveMember==G->Party.IndexOfByKey(Planned)
                ? FLinearColor(.95f,.78f,.32f,.82f) : FLinearColor(.42f,.9f,.62f,.72f);
            for (const FVector Destination:Destinations)
            {
                const auto* NavPath=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),From,Destination,Planned);
                if (NavPath && NavPath->IsValid() && !NavPath->IsPartial())
                {
                    for (int32 Point=1;Point<NavPath->PathPoints.Num();++Point)
                    {
                        FVector2D A,B;
                        if (PC->ProjectWorldLocationToScreen(NavPath->PathPoints[Point-1]+FVector(0,0,10),A) &&
                            PC->ProjectWorldLocationToScreen(NavPath->PathPoints[Point]+FVector(0,0,10),B))
                            DrawLine(A.X,A.Y,B.X,B.Y,PathColor,2.2f);
                    }
                    WorldRing(Destination,18,PathColor);
                }
                From=Destination;
            }
        }
    }
    const FVector Points[]={G->TNTLocation,G->DetonatorLocation,G->ExitLocation,G->HideLocation};
    const TCHAR* Names[]={TEXT("TNT"),TEXT("Detonator"),TEXT("Vychod"),TEXT("Ukryt")};
    for (int32 I=0;I<4;++I)
    {
        if ((I==0 && G->bHasTNT) || (I==1 && G->bBridgeDestroyed)) continue;
        FVector2D P;
        if (PC->ProjectWorldLocationToScreen(Points[I]+FVector(0,0,65),P))
        {
            const float Marker=Compact ? 12 : 20;
            Icon(TEXT("hand"),P.X-Marker/2,P.Y-Marker/2,Marker,FLinearColor(.94f,.84f,.54f));
            if (!Compact) Label(Names[I],P.X+12,P.Y-5,.85f);
        }
    }
    const float X=W-16-7*(S+4),Y=H-16-2*(S+4);
    auto* Unit=G->Party[G->ActiveMember].Get();
    Button(G->bTacticalPause ? TEXT("play") : TEXT("pause"),X,Y,S,G->bTacticalPause ? TEXT("Pokracovat (Space)") : TEXT("Takticka pauza (Space)"),[G](){ G->TogglePause(); },G->bTacticalPause);
    Button(TEXT("swords"),X+(S+4),Y,S,TEXT("Tichy utok (T)"),[PC](){ PC->Arm(EOKOrder::Takedown); },PC->bArmed && PC->ArmedOrder==EOKOrder::Takedown);
    Button(TEXT("circle_dot"),X+2*(S+4),Y,S,TEXT("Odlakanie (F)"),[PC](){ PC->Arm(EOKOrder::Distract); },PC->bArmed && PC->ArmedOrder==EOKOrder::Distract);
    Button(TEXT("hand"),X+3*(S+4),Y,S,TEXT("Interakcia (E)"),[G](){ FOKRTOrder O; O.Kind=EOKOrder::Interact; G->Command(O); });
    Button(TEXT("backpack"),X+4*(S+4),Y,S,TEXT("Preniest / polozit telo (B)"),[PC](){ PC->Arm(EOKOrder::Carry); });
    Button(TEXT("eye"),X+5*(S+4),Y,S,TEXT("Zorne kuzele"),[G](){ G->bCones=!G->bCones; for (AOKRTUnit* U:G->Enemies) U->Vision->bConeVisible=G->bCones; },G->bCones);
    Button(TEXT("menu"),X+6*(S+4),Y,S,TEXT("Menu (Esc)"),[G](){ G->ToggleMenu(); });
    const FName StanceIcons[]={TEXT("footprints"),TEXT("move_right"),TEXT("chevron_down"),TEXT("minus")};
    const TCHAR* StanceNames[]={TEXT("Chodza (W)"),TEXT("Beh (R)"),TEXT("Prikrcenie (C)"),TEXT("Plazenie (V)")};
    for (int32 I=0;I<4;++I)
        Button(StanceIcons[I],X+I*(S+4),Y+S+4,S,StanceNames[I],[PC,I](){ PC->SetPartyStance(static_cast<EOKStance>(I)); },static_cast<int32>(Unit->Stance)==I);
    Button(TEXT("rotate_ccw"),X+4*(S+4),Y+S+4,S,TEXT("Otocit kameru"),[G](){ G->Orbit(G->bSteppedCamera ? -45 : -8,0); });
    Button(TEXT("rotate_cw"),X+5*(S+4),Y+S+4,S,TEXT("Otocit kameru"),[G](){ G->Orbit(G->bSteppedCamera ? 45 : 8,0); });
    Button(TEXT("minus"),X+6*(S+4),Y+S+4,S,TEXT("Zrusit prikazy (X)"),[G,PC](){ PC->bArmed=false; for (AOKRTUnit* U:G->Party) if (U->IsSelected()) U->CancelOrders(); });
    const float MessageW=Compact ? W-32 : FMath::Min(490.f,X-32);
    const float MessageY=Compact ? Y-52 : H-68;
    DrawRect(FLinearColor(.025f,.03f,.035f,.82f),16,MessageY,MessageW,44);
    Panels.Add(FBox2D(FVector2D(16,MessageY),FVector2D(16+MessageW,MessageY+44)));
    Wrap(Unit->Cooldown>0 ? FString::Printf(TEXT("%s | %.1fs"),*G->Message,Unit->Cooldown) : G->Message,24,MessageY+10,MessageW-16,.95f);
    if (G->bMenu || G->bWon || G->bLost)
    {
        DrawRect(FLinearColor(0,0,0,.55f),0,0,W,H);
        Buttons.Reset(); Panels.Reset(); Panels.Add(FBox2D(FVector2D(0,0),FVector2D(W,H)));
        const float MW=FMath::Min(400.f,W-40),MX=(W-MW)/2,MY=FMath::Max(20.f,(H-300)/2);
        DrawRect(FLinearColor(.03f,.035f,.04f,.98f),MX,MY,MW,300);
        Label(G->bOptions ? TEXT("Nastavenia") : G->bWon ? TEXT("Misia splnena") : G->bLost ? TEXT("Misia zlyhala") : TEXT("Operacia Kopanice"),MX+20,MY+20,1.5f,FLinearColor(.91f,.8f,.52f));
        if (G->bOptions)
        {
            auto Row=[&](FName Name,float DY,const FString& Text,TFunction<void()> Action,bool Active)
            { Button(Name,MX+20,MY+DY,40,Text,MoveTemp(Action),Active); Label(Text,MX+72,MY+DY+12,1); };
            Row(TEXT("eye"),70,TEXT("Zorne kuzele"),[G](){ G->bCones=!G->bCones; for (AOKRTUnit* U:G->Enemies) U->Vision->bConeVisible=G->bCones; },G->bCones);
            Row(TEXT("footprints"),118,TEXT("Nahlad cesty"),[G](){ G->bPathPreview=!G->bPathPreview; },G->bPathPreview);
            Row(TEXT("rotate_cw"),166,TEXT("Kamera po 45 stupnoch"),[G](){ G->bSteppedCamera=!G->bSteppedCamera; },G->bSteppedCamera);
            Row(TEXT("sun"),214,FString::Printf(TEXT("Kvalita: %d"),G->Quality),[G](){ G->Quality=(G->Quality+1)%4; Scalability::FQualityLevels L; L.SetFromSingleQualityLevel(G->Quality); Scalability::SetQualityLevels(L); },false);
            Button(TEXT("undo_2"),MX+MW-60,MY+252,36,TEXT("Spat"),[G](){ G->bOptions=false; });
        }
        else
        {
            const bool Ended=G->bWon || G->bLost;
            Button(TEXT("play"),MX+20,MY+80,44,Ended ? TEXT("Nova misia") : TEXT("Pokracovat"),[G,Ended](){ if (Ended) G->Restart(); else G->ToggleMenu(); }); Label(Ended ? TEXT("Nova misia") : TEXT("Pokracovat"),MX+80,MY+93,1.1f);
            Button(TEXT("settings"),MX+20,MY+140,44,TEXT("Nastavenia"),[G](){ G->bOptions=true; }); Label(TEXT("Nastavenia"),MX+80,MY+153,1.1f);
            Button(TEXT("menu"),MX+20,MY+200,44,TEXT("Ukoncit hru"),[PC](){ PC->ConsoleCommand(TEXT("quit")); }); Label(TEXT("Ukoncit"),MX+80,MY+213,1.1f);
        }
    }
    float MouseX,MouseY;
    if (PC->GetMousePosition(MouseX,MouseY))
        for (const auto& B:Buttons) if (B.Bounds.IsInsideOrOn(FVector2D(MouseX,MouseY)))
        {
            const float TW=FMath::Min(290.f,W-32),TX=FMath::Clamp(MouseX,16.f,W-TW-16),TY=FMath::Clamp(MouseY-48,16.f,H-70);
            DrawRect(FLinearColor(.02f,.02f,.02f,.96f),TX,TY,TW,44); Wrap(B.Tip,TX+8,TY+8,TW-16,.9f); break;
        }
}
