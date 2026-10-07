#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTUnit.h"

namespace
{
    struct FOrderDisplay { FName Icon; FString Name; };
    FOrderDisplay Describe(const FOKRTOrder& Order)
    {
        switch (Order.Kind)
        {
        case EOKOrder::Move: return {Order.bRunToDestination ? TEXT("move_right") : TEXT("footprints"),
            Order.bRunToDestination ? TEXT("Beh") : TEXT("Presun")};
        case EOKOrder::Takedown: return {TEXT("swords"),TEXT("Tichy utok")};
        case EOKOrder::Distract: return {TEXT("circle_dot"),TEXT("Odlakanie")};
        case EOKOrder::Carry: return {TEXT("backpack"),Order.Target.IsExplicitlyNull() ? TEXT("Polozit telo") : TEXT("Preniest telo")};
        case EOKOrder::Stance:
            switch (Order.Stance)
            {
            case EOKStance::Walk: return {TEXT("footprints"),TEXT("Chodza")};
            case EOKStance::Run: return {TEXT("move_right"),TEXT("Beh")};
            case EOKStance::Crouch: return {TEXT("chevron_down"),TEXT("Prikrcenie")};
            case EOKStance::Prone: return {TEXT("minus"),TEXT("Plazenie")};
            }
            break;
        case EOKOrder::Interact:
            switch (Order.Interaction)
            {
            case EOKInteraction::CollectTNT: return {TEXT("hand"),TEXT("Ziskat TNT")};
            case EOKInteraction::DetonateBridge: return {TEXT("hand"),TEXT("Odpalit most")};
            case EOKInteraction::HideBody: return {TEXT("hand"),TEXT("Ukryt telo")};
            case EOKInteraction::CollectDocuments: return {TEXT("hand"),TEXT("Ziskat dokumenty")};
            case EOKInteraction::SabotageCommandPost: return {TEXT("hand"),TEXT("Sabotaz velitelstva")};
            default: return {TEXT("hand"),TEXT("Interakcia")};
            }
        }
        return {TEXT("clock_3"),TEXT("Rozkaz")};
    }
}

void AOKRTHUD::DrawOrderQueue(AOKRTGameMode* G,FVector2D Position,float Width)
{
    auto* Unit=G->Party[G->ActiveMember].Get();
    if (QueueUnit.Get()!=Unit) { QueueUnit=Unit; QueuePage=0; }
    const int32 Count=Unit->QueueSize(),Pages=FMath::Max(1,FMath::DivideAndRoundUp(Count,5));
    QueuePage=FMath::Clamp(QueuePage,0,Pages-1);
    if (!Unit->IsAlive() || Count==0) return;
    const FBox2D Bounds(Position,Position+FVector2D(Width,70));
    if (Bounds.Min.Y<0) return;
    auto Intersects=[&](const FBox2D& B)
    { return Bounds.Min.X<B.Max.X && Bounds.Max.X>B.Min.X && Bounds.Min.Y<B.Max.Y && Bounds.Max.Y>B.Min.Y; };
    for (const auto& Panel:Panels) if (Intersects(Panel)) return;
    for (const auto& Control:Buttons) if (Intersects(Control.Bounds)) return;
    Panels.Add(Bounds);
    DrawRect(FLinearColor(.025f,.03f,.035f,.88f),Position.X,Position.Y,Width,70);
    Label(FString::Printf(TEXT("%s | %d"),*Unit->DisplayName,Count),Position.X+8,Position.Y+5,.9f,FLinearColor(.91f,.8f,.52f));
    Label(FString::Printf(TEXT("%d/%d"),QueuePage+1,Pages),Position.X+Width-38,Position.Y+5,.85f);
    const float X=Position.X+(Width-332)/2,Y=Position.Y+26;
    Button(TEXT("arrow_left"),X,Y,38,TEXT("Predosle rozkazy"),[this](){ QueuePage=FMath::Max(0,QueuePage-1); },false,QueuePage>0);
    Buttons.Last().Id=TEXT("QueuePrevious");
    for (int32 Slot=0;Slot<5;++Slot)
    {
        const int32 Index=QueuePage*5+Slot;
        FOKRTOrder Order;
        if (!Unit->GetQueuedOrder(Index,Order)) continue;
        const auto Display=Describe(Order);
        FString Tip=FString::Printf(TEXT("%d. %s"),Index+1,*Display.Name);
        if (Order.Target.IsValid()) Tip+=TEXT(" / ")+Order.Target->DisplayName;
        if (Index==0 && Unit->HasStartedOrder()) Tip+=G->bTacticalPause ? TEXT(" / pozastavene") : TEXT(" / vykonava sa");
        Button(Display.Icon,X+(Slot+1)*42,Y,38,Tip,[G,Weak=TWeakObjectPtr<AOKRTUnit>(Unit),Index]()
        {
            FOKRTOrder Current;
            if (auto* U=Weak.Get(); U && G->Party[G->ActiveMember]==U && U->GetQueuedOrder(Index,Current))
                G->Message=FString::Printf(TEXT("%d. %s"),Index+1,*Describe(Current).Name);
        },Index==0 && Unit->HasStartedOrder());
        Buttons.Last().Id=FName(*FString::Printf(TEXT("QueuedOrder%d"),Index));
        Label(FString::FromInt(Index+1),X+(Slot+1)*42+24,Y+1,.7f,FLinearColor(.84f,.9f,.86f));
    }
    Button(TEXT("arrow_right"),X+6*42,Y,38,TEXT("Dalsie rozkazy"),[this,Pages](){ QueuePage=FMath::Min(Pages-1,QueuePage+1); },false,QueuePage<Pages-1);
    Buttons.Last().Id=TEXT("QueueNext");
    const bool CanUndo=Unit->CanUndoLastOrder();
    Button(TEXT("undo_2"),X+7*42,Y,38,CanUndo ? TEXT("Zrusit posledny cakajuci rozkaz (Backspace)") :
        G->bTacticalPause ? TEXT("Prebiehajuci rozkaz: zastavit cez X") : TEXT("Uprava planu vyzaduje takticku pauzu"),
        [PC=CastChecked<AOKRTPlayerController>(PlayerOwner)](){ PC->UndoLastOrder(); },false,CanUndo);
    Buttons.Last().Id=TEXT("UndoOrder");
}
