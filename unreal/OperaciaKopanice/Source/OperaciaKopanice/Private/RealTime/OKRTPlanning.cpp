#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "AIController.h"
#include "Components/SplineComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void AOKRTGameMode::ToggleCommandMode()
{
    if (bInitialPausePending || bMenu || bWon || bLost) return;
    if (bCommandMode) { CancelPlan(); return; }
    if (ActiveSyncBatch) { Message=TEXT("Tim este vykonava synchronizovany plan."); return; }
    bPlanPriorPause=bTacticalPause; bCommandMode=true; bTacticalPause=true;
    Drafts.SetNum(Party.Num()); DraftValid.Init(false,Party.Num());
    UGameplayStatics::SetGamePaused(GetWorld(),true);
    Message=TEXT("Synchronizovany plan pripraveny.");
}
void AOKRTGameMode::CancelPlan()
{
    if (!bCommandMode) return;
    DraftValid.Reset(); Drafts.Reset(); bCommandMode=false;
    bTacticalPause=bPlanPriorPause;
    UGameplayStatics::SetGamePaused(GetWorld(),bTacticalPause || bMenu);
    PreviewSpline->ClearSplinePoints();
    Message=TEXT("Plan zruseny. Povodne rozkazy zachovane.");
}
bool AOKRTGameMode::GetDraft(int32 Index,FOKRTOrder& Order) const
{
    if (!bCommandMode || !DraftValid.IsValidIndex(Index) || !DraftValid[Index]) return false;
    Order=Drafts[Index]; return true;
}
bool AOKRTGameMode::StagePlan(AOKRTUnit* Unit,const FOKRTOrder& Order)
{
    const int32 Index=Party.IndexOfByKey(Unit);
    if (!bCommandMode || !Drafts.IsValidIndex(Index) || !Unit->IsAlive()) return false;
    if (Order.Kind==EOKOrder::Stance)
    { Message=TEXT("Postoj nastav pred otvorenim synchronizovaneho planu."); return false; }
    FString Reason; TArray<FVector> Points;
    if (!Unit->ValidateOrder(Order,false,false,Reason) || !Unit->GetOrderPath(Order,false,Points,Reason))
    { Message=Reason; return false; }
    if (Order.Kind==EOKOrder::Interact && (!Order.bApproachInteraction ||
        (Order.Interaction!=EOKInteraction::HideBody && !OKMissions::CanExecute(Mission(),WorldState(),Order.Interaction))))
    { Message=TEXT("Ciel este nie je odomknuty."); return false; }
    Drafts[Index]=Order; Drafts[Index].SyncBatch=0; DraftValid[Index]=true;
    return true;
}
bool AOKRTGameMode::ExecutePlan()
{
    if (!bCommandMode || bMenu || bWon || bLost) return false;
    int32 Count=0;
    // Validate the whole transaction before cancelling any existing unit plan.
    for (int32 I=0;I<Party.Num();++I) if (DraftValid[I])
    {
        FString Reason; TArray<FVector> Points;
        if (!Party[I]->ValidateOrder(Drafts[I],false,false,Reason) || !Party[I]->GetOrderPath(Drafts[I],false,Points,Reason))
        { Message=Reason; return false; }
        for (int32 J=0;J<I;++J)
        {
            if (DraftValid[J] && Drafts[I].Target.IsValid() && Drafts[I].Target==Drafts[J].Target &&
                (Drafts[I].Kind==EOKOrder::Takedown || Drafts[I].Kind==EOKOrder::Carry || Drafts[I].Kind==EOKOrder::Heal))
            { Message=TEXT("Dve akcie nemozu pouzit rovnaky ciel."); return false; }
            if (DraftValid[J] && Drafts[I].Kind==EOKOrder::Interact && Drafts[J].Kind==EOKOrder::Interact &&
                Drafts[I].Interaction==Drafts[J].Interaction && Drafts[I].Interaction!=EOKInteraction::HideBody)
            { Message=TEXT("Ciel misie moze aktivovat iba jeden clen planu."); return false; }
        }
        ++Count;
    }
    if (!Count) { Message=TEXT("Plan nema ziadne akcie."); return false; }
    ActiveSyncBatch=NextSyncBatch++; bSyncReleased=false; SyncParticipants=DraftValid;
    for (int32 I=0;I<Party.Num();++I) if (DraftValid[I])
    {
        Party[I]->CancelOrders();
        auto Order=Drafts[I]; Order.SyncBatch=ActiveSyncBatch;
        Party[I]->Orders.Add(Order);
    }
    bCommandMode=false; Drafts.Reset(); DraftValid.Reset();
    bTacticalPause=false; UGameplayStatics::SetGamePaused(GetWorld(),false);
    Message=TEXT("Tim sa presuva na synchronizovane akcie."); return true;
}
bool AOKRTGameMode::SyncReleased(int32 Batch) const
{ return Batch==0 || (Batch==ActiveSyncBatch && bSyncReleased); }
void AOKRTGameMode::CancelSync(int32 Batch)
{
    if (!Batch || Batch!=ActiveSyncBatch) return;
    ActiveSyncBatch=0; bSyncReleased=false; SyncParticipants.Reset();
    for (AOKRTUnit* Unit:Party)
    {
        if (!Unit->Orders.ContainsByPredicate([Batch](const auto& O){ return O.SyncBatch==Batch; })) continue;
        if (!Unit->Orders.IsEmpty() && Unit->Orders[0].SyncBatch==Batch)
            if (auto* AI=Cast<AAIController>(Unit->GetController())) AI->StopMovement();
        Unit->Orders.RemoveAll([Batch](const auto& O){ return O.SyncBatch==Batch; });
        Unit->ReadyBatch=0; Unit->bOrderStarted=false;
    }
    Message=TEXT("Synchronizovana akcia zrusena. Ciel alebo cesta sa zmenili.");
}
void AOKRTGameMode::UpdateSynchronization()
{
    if (!ActiveSyncBatch) return;
    bool Ready=true,Pending=false;
    for (int32 I=0;I<Party.Num();++I) if (SyncParticipants.IsValidIndex(I) && SyncParticipants[I])
    {
        auto* Unit=Party[I].Get();
        if (!Unit->IsAlive()) { CancelSync(ActiveSyncBatch); return; }
        const bool HasOrder=Unit->Orders.ContainsByPredicate([this](const auto& O){ return O.SyncBatch==ActiveSyncBatch; });
        if (!bSyncReleased && !HasOrder) { CancelSync(ActiveSyncBatch); return; }
        Pending|=HasOrder;
        Ready&=Unit->IsSyncReady(ActiveSyncBatch);
    }
    if (!bSyncReleased && Ready) { bSyncReleased=true; Message=TEXT("Synchronizovane akcie vykonavane."); }
    if (!Pending) { ActiveSyncBatch=0; SyncParticipants.Reset(); bSyncReleased=false; }
}
bool AOKRTUnit::AwaitSynchronization(const FOKRTOrder& Order)
{
    if (!Order.SyncBatch) return false;
    ReadyBatch=Order.SyncBatch;
    const auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    return Game && !Game->SyncReleased(Order.SyncBatch);
}
bool AOKRTUnit::IsSyncReady(int32 Batch) const
{
    if (ReadyBatch!=Batch || Orders.IsEmpty() || Orders[0].SyncBatch!=Batch || !IsAlive()) return false;
    FString Reason;
    const auto& O=Orders[0];
    if (!ValidateOrder(O,true,true,Reason)) return false;
    if (O.Kind==EOKOrder::Distract) return Cooldown<=0;
    if (O.Kind==EOKOrder::Carry && O.Target.IsExplicitlyNull()) return CarriedBody.IsValid();
    const auto* Target=O.Target.Get();
    const FVector Destination=Target ? Target->Feet() : O.Location;
    const float Reach=Target ? 145.f : O.Kind==EOKOrder::Interact ? 120.f : 70.f;
    if (FVector::Dist2D(Feet(),Destination)>Reach) return false;
    if (O.Kind==EOKOrder::Takedown || O.Kind==EOKOrder::Heal)
    {
        if (Cooldown>0 || !Target) return false;
        if (O.Kind==EOKOrder::Takedown && FVector::DotProduct(Target->GetActorForwardVector(),(Feet()-Target->Feet()).GetSafeNormal2D())>=.1f) return false;
    }
    if (Target)
    {
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(OKSyncReach),false,this); Params.AddIgnoredActor(Target);
        if (FMath::Abs(Feet().Z-Target->Feet().Z)>75 || GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Params)) return false;
    }
    if (O.Kind==EOKOrder::Interact && O.Interaction!=EOKInteraction::HideBody)
        if (const auto* G=GetWorld()->GetAuthGameMode<AOKRTGameMode>(); !G || !OKMissions::CanExecute(G->Mission(),G->WorldState(),O.Interaction)) return false;
    return true;
}
FBox2D AOKRTGameMode::TacticalBounds() const
{ return MissionId==0 ? FBox2D(FVector2D(-90,-90),FVector2D(1890,1530)) : FBox2D(FVector2D::ZeroVector,FVector2D(Mission().MapSize)); }
bool AOKRTGameMode::TeamSees(const AOKRTUnit* Enemy) const
{
    for (const AOKRTUnit* Unit:Party) { bool Near=false; if (Unit->IsAlive() && Unit->Vision->Sees(Enemy,Near)) return true; }
    return false;
}
void AOKRTGameMode::FocusWorld(FVector Location)
{
    if (Location.ContainsNaN()) return;
    const auto Bounds=TacticalBounds();
    CameraTarget=FVector(FMath::Clamp(Location.X,Bounds.Min.X,Bounds.Max.X),FMath::Clamp(Location.Y,Bounds.Min.Y,Bounds.Max.Y),0);
    UpdateCamera();
}
