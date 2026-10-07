#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "Engine/World.h"

TArray<AOKRTHUD::FPartyMarker> AOKRTHUD::BuildPartyMarkers() const
{
    TArray<FPartyMarker> Result;
    const auto* G=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!PlayerOwner || !G || !G->bPartyMarkers || G->bMenu || G->bWon || G->bLost) return Result;
    int32 W=0,H=0; PlayerOwner->GetViewportSize(W,H);
    if (W<=0 || H<=0 || !RenderSize.Equals(FVector2D(W,H),.1)) return Result;
    const float Size=W<800 ? 44.f : 32.f;
    TArray<FBox2D> Reserved;
    auto Reserve=[&](FVector Bottom,FVector Top,float Padding)
    {
        FVector2D A,B;
        if (PlayerOwner->ProjectWorldLocationToScreen(Bottom,A) && PlayerOwner->ProjectWorldLocationToScreen(Top,B))
        {
            FBox2D Bounds(ForceInit); Bounds+=A; Bounds+=B;
            Reserved.Add(Bounds.ExpandBy(Padding));
        }
    };
    auto ReserveUnit=[&](const AOKRTUnit* Unit)
    {
        if (!Unit || Unit->IsHidden()) return;
        FVector Head; FRotator Facing; Unit->GetActorEyesViewPoint(Head,Facing);
        Reserve(Unit->Feet(),Head+FVector(0,0,35),W<800 ? 12.f : 8.f);
    };
    for (const auto& Unit:G->Party) ReserveUnit(Unit.Get());
    for (const auto& Unit:G->Enemies) ReserveUnit(Unit.Get());
    for (const FVector Point:{G->TNTLocation,G->DetonatorLocation,G->ExitLocation,G->HideLocation})
        Reserve(Point,Point+FVector(0,0,65),W<800 ? 12.f : 14.f);
    auto Intersects=[](const FBox2D& A,const FBox2D& B)
    { return A.Min.X<B.Max.X && A.Max.X>B.Min.X && A.Min.Y<B.Max.Y && A.Max.Y>B.Min.Y; };
    auto Clear=[&](const FBox2D& Bounds)
    {
        if (Bounds.Min.X<4 || Bounds.Min.Y<4 || Bounds.Max.X>W-4 || Bounds.Max.Y>H-4) return false;
        for (const auto& Panel:Panels) if (Intersects(Bounds,Panel)) return false;
        for (const auto& Button:Buttons) if (Intersects(Bounds,Button.Bounds)) return false;
        for (const auto& Target:Reserved) if (Intersects(Bounds,Target)) return false;
        for (const auto& Marker:Result) if (Intersects(Bounds,Marker.Bounds)) return false;
        return true;
    };
    for (int32 I=0;I<G->Party.Num();++I)
    {
        const auto* Unit=G->Party[I].Get();
        if (!Unit || Unit->bEnemy || !Unit->IsAlive() || Unit->IsHidden()) continue;
        FVector Head; FRotator Facing; Unit->GetActorEyesViewPoint(Head,Facing);
        FVector2D Anchor;
        if (!PlayerOwner->ProjectWorldLocationToScreen(Head+FVector(0,0,35),Anchor) ||
            Anchor.X<0 || Anchor.Y<0 || Anchor.X>=W || Anchor.Y>=H || OverUI(Anchor)) continue;
        // Recompute from the current view for both drawing and picking. Offset
        // neighbouring markers instead of allowing one to steal the other's hit.
        const FVector2D Offsets[]={{0,0},{Size+8,0},{-Size-8,0},{0,-Size-8},{0,Size+16},
            {Size+8,-Size-8},{-Size-8,-Size-8},{Size+8,Size+16},{-Size-8,Size+16},{2*(Size+8),0}};
        for (const FVector2D Offset:Offsets)
        {
            const FVector2D Min=Anchor+FVector2D(-Size/2,-Size-8)+Offset;
            const FBox2D Bounds(Min,Min+FVector2D(Size,Size));
            if (!Clear(Bounds)) continue;
            Result.Add({I,Bounds,Anchor}); break;
        }
    }
    return Result;
}
bool AOKRTHUD::FindPartyMarker(int32 Member,FVector2D& Center) const
{
    for (const auto& Marker:BuildPartyMarkers())
        if (Marker.Member==Member) { Center=Marker.Bounds.GetCenter(); return true; }
    return false;
}
int32 AOKRTHUD::PartyMarkerAt(FVector2D Point) const
{
    if (Point.ContainsNaN()) return INDEX_NONE;
    for (const auto& Marker:BuildPartyMarkers())
        if (Marker.Bounds.IsInsideOrOn(Point)) return Marker.Member;
    return INDEX_NONE;
}
void AOKRTHUD::DrawPartyMarkers()
{
    const auto* G=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!G) return;
    float MX=0,MY=0; const bool Mouse=PlayerOwner->GetMousePosition(MX,MY);
    for (const auto& Marker:BuildPartyMarkers())
    {
        const auto* Unit=G->Party[Marker.Member].Get();
        const bool Active=Unit->IsSelected() && G->ActiveMember==Marker.Member;
        const FLinearColor Color=Active ? FLinearColor(.91f,.8f,.52f) : Unit->IsSelected() ?
            FLinearColor(.65f,.9f,.73f) : FLinearColor(.76f,.82f,.81f);
        const FVector2D A=Marker.Bounds.Min,B=Marker.Bounds.Max;
        const float Size=B.X-A.X;
        const bool Hover=Mouse && Marker.Bounds.IsInsideOrOn(FVector2D(MX,MY));
        const FVector2D Edge(FMath::Clamp(Marker.Anchor.X,A.X,B.X),FMath::Clamp(Marker.Anchor.Y,A.Y,B.Y));
        DrawLine(Edge.X,Edge.Y,Marker.Anchor.X,Marker.Anchor.Y,Color,1);
        DrawRect(Hover ? FLinearColor(.13f,.18f,.17f,.98f) : FLinearColor(.025f,.035f,.035f,.9f),A.X,A.Y,Size,Size);
        DrawLine(A.X,A.Y,B.X,A.Y,Color,Active ? 2 : 1);
        DrawLine(A.X,B.Y,B.X,B.Y,Color,1);
        DrawLine(A.X,A.Y,A.X,B.Y,Color,1); DrawLine(B.X,A.Y,B.X,B.Y,Color,1);
        Icon(TEXT("crosshair"),A.X+Size*.12f,A.Y+Size*.29f,Size*.42f,Color);
        Label(FString::FromInt(Marker.Member+1),A.X+Size*.6f,A.Y+Size/2-7,Size>32 ? 1.15f : 1.f,Color);
    }
}
