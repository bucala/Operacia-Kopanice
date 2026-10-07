#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTUnit.h"
#include "Engine/World.h"

FOKRTOrder AOKRTGameMode::IndividualOrder(const FOKRTOrder& Order,const AOKRTUnit* Unit) const
{
    FOKRTOrder Result=Order;
    if (Order.Kind!=EOKOrder::Move) return Result;
    const auto Selected=Party.FilterByPredicate([](const auto& U){ return U->IsSelected() && U->IsAlive(); });
    if (Selected.Num()>1) Result.Location.Y+=Selected.IndexOfByKey(Unit)==0 ? -45 : 45;
    return Result;
}
float AOKRTGameMode::CommandFeedbackOpacity() const
{
    if (bMenu || bWon || bLost || !CommandFeedback.bWorldTarget || CommandFeedback.Requested==0 ||
        CommandFeedback.Location.ContainsNaN()) return 0;
    const double Age=FPlatformTime::Seconds()-CommandFeedback.IssuedAt;
    if (Age<0 || Age>=2.4) return 0;
    return static_cast<float>(FMath::Min(1.0,(2.4-Age)/.8));
}
bool AOKRTHUD::FindCommandFeedback(FVector2D& P) const
{
    const auto* G=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!G || G->CommandFeedbackOpacity()<=0 || !PlayerOwner) return false;
    const auto& Feedback=G->CommandFeedback;
    if (!PlayerOwner->ProjectWorldLocationToScreen(Feedback.Location+FVector(0,0,18),P)) return false;
    const FBox2D Bounds(P-FVector2D(28,28),P+FVector2D(42,28));
    if (Bounds.Min.X<0 || Bounds.Min.Y<0 || Bounds.Max.X>RenderSize.X || Bounds.Max.Y>RenderSize.Y) return false;
    auto Intersects=[&](const FBox2D& B)
    { return Bounds.Min.X<B.Max.X && Bounds.Max.X>B.Min.X && Bounds.Min.Y<B.Max.Y && Bounds.Max.Y>B.Min.Y; };
    for (const auto& Panel:Panels) if (Intersects(Panel)) return false;
    for (const auto& Control:Buttons) if (Intersects(Control.Bounds)) return false;
    return true;
}
void AOKRTHUD::DrawCommandFeedback()
{
    FVector2D P;
    if (!FindCommandFeedback(P)) return;
    const auto* G=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    const auto& Feedback=G->CommandFeedback;
    const float Opacity=G->CommandFeedbackOpacity();
    const FLinearColor Color=Feedback.Accepted==0 ? FLinearColor(.95f,.24f,.2f,Opacity) :
        Feedback.Accepted<Feedback.Requested ? FLinearColor(.98f,.73f,.24f,Opacity) : FLinearColor(.4f,.91f,.62f,Opacity);
    const float Radius=18+6*(1-Opacity);
    for (int32 I=0;I<32;++I)
    {
        const float A=2*PI*I/32,B=2*PI*(I+1)/32;
        DrawLine(P.X+FMath::Cos(A)*Radius,P.Y+FMath::Sin(A)*Radius,
            P.X+FMath::Cos(B)*Radius,P.Y+FMath::Sin(B)*Radius,Color,1.2f);
    }
    const FName Symbol=Feedback.Accepted==0 ? TEXT("minus") : Feedback.Kind==EOKOrder::Move ? TEXT("footprints") :
        Feedback.Kind==EOKOrder::Takedown ? TEXT("swords") : Feedback.Kind==EOKOrder::Carry ? TEXT("backpack") :
        Feedback.Kind==EOKOrder::Distract ? TEXT("circle_dot") : TEXT("hand");
    DrawRect(FLinearColor(.02f,.03f,.03f,.8f*Opacity),P.X-12,P.Y-12,24,24);
    Icon(Symbol,P.X-10,P.Y-10,20,Color);
    if (Feedback.Requested>1) Label(FString::Printf(TEXT("%d/%d"),Feedback.Accepted,Feedback.Requested),P.X+15,P.Y-7,.85f,Color);
}
