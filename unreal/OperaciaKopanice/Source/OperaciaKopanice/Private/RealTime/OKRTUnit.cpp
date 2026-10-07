#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTVisionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"

AOKRTUnit::AOKRTUnit()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(30,90);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,540,0);
    GetCharacterMovement()->bUseRVOAvoidance=true;
    GetCharacterMovement()->AvoidanceConsiderationRadius=400;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true;
    GetCharacterMovement()->SetCrouchedHalfHeight(45);
    bUseControllerRotationYaw=false;
    AIControllerClass=AOKRTGuardController::StaticClass();
    AutoPossessAI=EAutoPossessAI::Disabled;
    Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SuppliedCharacter"));
    Visual->SetupAttachment(GetRootComponent());
    Visual->SetCollisionProfileName(TEXT("NoCollision"));
    Visual->SetCanEverAffectNavigation(false);
    AnimatedVisual=CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("AnimatedCharacter"));
    AnimatedVisual->SetupAttachment(Visual);
    AnimatedVisual->SetCollisionProfileName(TEXT("NoCollision"));
    AnimatedVisual->SetCanEverAffectNavigation(false);
    AnimatedVisual->SetBoundsScale(1.6f);
    Vision=CreateDefaultSubobject<UOKRTVisionComponent>(TEXT("VisionCone"));
    Vision->SetupAttachment(GetRootComponent());
    Stimuli=CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("SightStimulus"));
    Stimuli->RegisterForSense(UAISense_Sight::StaticClass());
}
void AOKRTUnit::Initialize(bool Guard,bool Officer)
{
    bEnemy=Guard;
    DisplayName=Guard ? TEXT("Hliadka") : Officer ? TEXT("Dostojnik") : TEXT("Partizan");
    const TCHAR* Path=Officer ? TEXT("/Game/Kopanice/Supplied/Officer/SM_OK_Officer.SM_OK_Officer") :
        TEXT("/Game/Kopanice/Supplied/Partisan/SM_OK_Partisan.SM_OK_Partisan");
    Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));
    const TCHAR* RigPath=Officer ? TEXT("/Game/Kopanice/Animated/Officer/SK_OK_Officer.SK_OK_Officer") :
        TEXT("/Game/Kopanice/Animated/Partisan/SK_OK_Partisan.SK_OK_Partisan");
    if (auto* Rig=LoadObject<USkeletalMesh>(nullptr,RigPath))
    {
        AnimatedVisual->SetSkinnedAssetAndUpdate(Rig);
        const FBoxSphereBounds Bounds=Rig->GetBounds();
        const float Scale=180.f/FMath::Max(1.f,float(Bounds.BoxExtent.Z*2));
        AnimatedVisual->SetRelativeScale3D(FVector(Scale));
        AnimatedVisual->SetRelativeLocation(FVector(0,0,-(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale));
        VisualHeight=Bounds.BoxExtent.Z*2*Scale;
        Visual->SetVisibility(false,false);
    }
    else
    {
        AnimatedVisual->SetVisibility(false);
        UE_LOG(LogTemp,Warning,TEXT("OK_RT_ANIMATION: missing rig for %s; static fallback"),*DisplayName);
    }
    UE_LOG(LogTemp,Display,TEXT("OK_RT_SIZE: %s enemy=%d height=%.1f cm"),*DisplayName,bEnemy,VisualHeight);
    SetStance(EOKStance::Walk);
    Vision->bConeVisible=Guard;
    Stimuli->RegisterWithPerceptionSystem();
    SpawnDefaultController();
}
FVector AOKRTUnit::Feet() const
{
    return GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}
void AOKRTUnit::GetQueuedLocations(TArray<FVector>& Locations) const
{
    Locations.Reset();
    for (const FOKRTOrder& Order:Orders)
    {
        if (Order.Kind==EOKOrder::Move || (Order.Kind==EOKOrder::Interact && Order.bApproachInteraction)) Locations.Add(Order.Location);
        else if ((Order.Kind==EOKOrder::Takedown || Order.Kind==EOKOrder::Carry) && Order.Target.IsValid())
            Locations.Add(Order.Target->Feet());
    }
}
void AOKRTUnit::GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const
{
    const float Height=!IsAlive() ? 20 : Stance==EOKStance::Prone ? 30 : Stance==EOKStance::Crouch ? 95 : 145;
    Location=Feet()+FVector(0,0,Height);
    Rotation=GetActorRotation();
}
void AOKRTUnit::SetStance(EOKStance Value)
{
    Stance=Value;
    const bool bLow=Value==EOKStance::Crouch || Value==EOKStance::Prone;
    if (!bLow) bInCover=false;
    if (bLow) Crouch(); else UnCrouch();
    float Speed=Value==EOKStance::Run ? 360 : Value==EOKStance::Walk ? 190 : Value==EOKStance::Crouch ? 90 : 50;
    if (CarriedBody.IsValid()) Speed=FMath::Min(Speed,100.f);
    GetCharacterMovement()->MaxWalkSpeed=Speed;
    GetCharacterMovement()->MaxWalkSpeedCrouched=Speed;
    if (!AnimatedVisual->GetSkinnedAsset())
    {
        Visual->SetRelativeLocation(FVector(Value==EOKStance::Prone ? -75 : 0,0,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3));
        Visual->SetRelativeRotation(FRotator(Value==EOKStance::Prone ? 85 : Value==EOKStance::Crouch ? 18 : 0,0,0));
    }
}
float AOKRTUnit::VisibilityFactor() const
{
    float Value=Stance==EOKStance::Prone ? .15f : Stance==EOKStance::Crouch ? .35f : 1.f;
    return bInCover ? Value*.3f : Value;
}
bool AOKRTUnit::ValidateOrder(const FOKRTOrder& Order,bool bExecuting,bool bKeepQueue,FString& Reason) const
{
    if (!IsAlive() || bEnemy) { Reason=TEXT("Postava nemoze prijat prikaz."); return false; }
    if (Order.Location.ContainsNaN()) { Reason=TEXT("Neplatna poloha ciela."); return false; }
    if (Order.Kind==EOKOrder::Distract && Distractions<=0)
    { Reason=TEXT("Ziadne predmety na odlakanie."); return false; }
    if (Order.Kind!=EOKOrder::Takedown && Order.Kind!=EOKOrder::Carry) return true;
    if (Order.Kind==EOKOrder::Carry && Order.Target.IsExplicitlyNull())
    {
        const bool bQueuedPickup=!bExecuting && bKeepQueue && Orders.ContainsByPredicate([](const FOKRTOrder& Pending)
        { return Pending.Kind==EOKOrder::Carry && Pending.Target.IsValid(); });
        if (CarriedBody.IsValid() || bQueuedPickup) return true;
        Reason=TEXT("Postava nenesie telo."); return false;
    }
    const auto* Target=Order.Target.Get();
    if (!Target || !Target->bEnemy || Target==this || Target->bHiddenBody || Target->Carrier.IsValid())
    { Reason=TEXT("Vyber dostupnu nepriatelsku hliadku."); return false; }
    if (Order.Kind==EOKOrder::Takedown)
    {
        if (!Target->IsAlive()) { Reason=TEXT("Hliadka je uz neutralizovana."); return false; }
        if (CarriedBody.IsValid()) { Reason=TEXT("Najprv poloz telo."); return false; }
    }
    else
    {
        const bool bQueuedTakedown=!bExecuting && bKeepQueue && Orders.ContainsByPredicate([Target](const FOKRTOrder& Pending)
        { return Pending.Kind==EOKOrder::Takedown && Pending.Target.Get()==Target; });
        if (Target->IsAlive() && !bQueuedTakedown)
        { Reason=TEXT("Najprv neutralizuj hliadku."); return false; }
        if (CarriedBody.IsValid()) { Reason=TEXT("Postava uz nesie telo."); return false; }
    }
    return true;
}
bool AOKRTUnit::Submit(const FOKRTOrder& Order,bool bAppend)
{
    auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    const bool bKeepQueue=bAppend || (Game && Game->bTacticalPause);
    FString Reason;
    if (!ValidateOrder(Order,false,bKeepQueue,Reason) || !CanNavigateOrder(Order,bKeepQueue,Reason))
    { if (Game) Game->Message=Reason; return false; }
    if (Order.Kind==EOKOrder::Stance && (!Game || !Game->bTacticalPause))
    { SetStance(Order.Stance); return true; }
    if (bKeepQueue && Orders.Num()>=32)
    { if (Game) Game->Message=TEXT("Front prikazov je plny."); return false; }
    if (!bKeepQueue)
    {
        Orders.Reset(); bOrderStarted=false;
        if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
    }
    Orders.Add(Order);
    return true;
}
bool AOKRTUnit::CanNavigateOrder(const FOKRTOrder& Order,bool bFromQueue,FString& Reason)
{
    const bool bTargetOrder=Order.Kind==EOKOrder::Takedown || (Order.Kind==EOKOrder::Carry && Order.Target.IsValid());
    if (Order.Kind!=EOKOrder::Move && !(Order.Kind==EOKOrder::Interact && Order.bApproachInteraction) && !bTargetOrder) return true;
    const FVector Destination=bTargetOrder ? Order.Target->Feet() : Order.Location;
    FVector From=Feet();
    if (bFromQueue)
    {
        TArray<FVector> Locations; GetQueuedLocations(Locations);
        if (!Locations.IsEmpty()) From=Locations.Last();
    }
    const float Tolerance=bTargetOrder ? 100.f : Order.Kind==EOKOrder::Interact ? 75.f : 30.f;
    auto Reject=[&Reason]()
    { Reason=TEXT("Ciel nema pristupnu cestu. Povodne prikazy zostali zachovane."); return false; };
    // Identical endpoints can produce an empty UE path. They still need a real
    // navigable point, including a pickup planned immediately after a takedown.
    if (From.Equals(Destination,1.f))
    {
        auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        FNavLocation Projected;
        if (Nav && Nav->ProjectPointToNavigation(Destination,Projected,FVector(Tolerance,Tolerance,75),
            &GetCharacterMovement()->GetNavAgentPropertiesRef()) &&
            FVector::Dist2D(Projected.Location,Destination)<=Tolerance && FMath::Abs(Projected.Location.Z-Destination.Z)<=75) return true;
        return Reject();
    }
    const auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),From,Destination,this);
    // UE may project an off-mesh goal onto a nearby bank or floor. Do not accept
    // that projected route as reaching a different clicked destination.
    if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.IsEmpty() ||
        FVector::Dist2D(Path->PathPoints.Last(),Destination)>Tolerance ||
        FMath::Abs(Path->PathPoints.Last().Z-Destination.Z)>75)
        return Reject();
    return true;
}
void AOKRTUnit::FinishOrder()
{
    if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
    if (Orders.Num()) Orders.RemoveAt(0);
    bOrderStarted=false;
}
bool AOKRTUnit::PromoteLastMoveToRun(FVector Destination)
{
    if (!IsAlive() || Orders.IsEmpty() || Orders.Last().Kind!=EOKOrder::Move ||
        FVector::Dist2D(Orders.Last().Location,Destination)>60) return false;
    FOKRTOrder Requested=Orders.Last(); Requested.Location=Destination;
    FString Reason;
    if (!CanNavigateOrder(Requested,true,Reason)) return false;
    Orders.Last().bRunToDestination=true;
    // Reapply pace once on execution/resume, including an already-started move.
    if (Orders.Num()==1) bOrderStarted=false;
    return true;
}
void AOKRTUnit::CancelOrders()
{
    Orders.Reset(); bOrderStarted=false;
    if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
}
void AOKRTUnit::UpdateOrders()
{
    auto* AI=Cast<AAIController>(GetController());
    auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!AI || !Game || Orders.IsEmpty()) return;
    const FOKRTOrder Order=Orders[0];
    FString Reason;
    // Recheck queued targets: another unit may have killed, carried or hidden them.
    if (!ValidateOrder(Order,true,true,Reason))
    { Game->Message=Reason; FinishOrder(); return; }
    if (Order.Kind==EOKOrder::Stance) { SetStance(Order.Stance); FinishOrder(); return; }
    if (Order.Kind==EOKOrder::Distract)
    {
        if (Cooldown>0) return;
        if (Game->Distract(this,Order.Location)) { --Distractions; Cooldown=6; }
        FinishOrder(); return;
    }
    if (Order.Kind==EOKOrder::Interact && !Order.bApproachInteraction)
    { Game->Interact(this,Order.Interaction); FinishOrder(); return; }
    if (Order.Kind==EOKOrder::Carry && Order.Target.IsExplicitlyNull()) { DropBody(); FinishOrder(); return; }
    const bool bTargetOrder=Order.Kind==EOKOrder::Carry || Order.Kind==EOKOrder::Takedown;
    auto* Target=Order.Target.Get();
    const FVector Destination=bTargetOrder ? Target->Feet() : Order.Location;
    const float Acceptance=bTargetOrder ? 100.f : Order.Kind==EOKOrder::Interact ? 75.f : 25.f;
    if (!bOrderStarted)
    {
        // Recheck once when a queued approach begins; topology may have changed
        // during tactical pause. Active paths remain owned by UE path following.
        if (!CanNavigateOrder(Order,false,Reason))
        { Game->Message=TEXT("Cesta k cielu uz nie je dostupna."); FinishOrder(); return; }
        if (Order.Kind==EOKOrder::Move)
        {
            if (Order.bRunToDestination) SetStance(EOKStance::Run);
        }
        const auto Result=bTargetOrder ? AI->MoveToActor(Target,Acceptance,false,true,true,nullptr,false) :
            AI->MoveToLocation(Destination,Acceptance,false,true,true,false,nullptr,false);
        bOrderStarted=true;
        if (Result==EPathFollowingRequestResult::Failed)
        { Game->Message=TEXT("Ciel nema pristupnu cestu."); FinishOrder(); return; }
    }
    if (AI->GetMoveStatus()!=EPathFollowingStatus::Idle) return;
    if (FVector::Dist2D(Feet(),Destination)>Acceptance+45)
    { Game->Message=TEXT("Prikaz nedosiahol ciel."); FinishOrder(); return; }
    if (bTargetOrder)
    {
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRTReach),false,this); Params.AddIgnoredActor(Target);
        if (FMath::Abs(Feet().Z-Target->Feet().Z)>75 ||
            GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Params))
        { Game->Message=TEXT("Prekazka blokuje pristup k cielu."); FinishOrder(); return; }
    }
    if (Order.Kind==EOKOrder::Takedown && Target)
    {
        if (Cooldown>0) return;
        const float Behind=FVector::DotProduct(Target->GetActorForwardVector(),(Feet()-Target->Feet()).GetSafeNormal2D());
        if (Behind<.1f)
        { Target->TakeHit(100); Cooldown=3; Game->Message=TEXT("Hliadka neutralizovana."); }
        else Game->Message=TEXT("Tichy utok vyzaduje pristup zozadu.");
    }
    if (Order.Kind==EOKOrder::Carry && Target && !Target->IsAlive())
    {
        CarriedBody=Target; Target->Carrier=this;
        Target->GetCapsuleComponent()->SetCollisionProfileName(TEXT("NoCollision"));
        Target->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
        Target->SetActorRelativeLocation(FVector(-35,0,50));
        SetStance(Stance);
    }
    if (Order.Kind==EOKOrder::Interact) Game->Interact(this,Order.Interaction);
    FinishOrder();
}
void AOKRTUnit::TakeHit(float Damage)
{
    if (!IsAlive()) return;
    Health=FMath::Max(0.f,Health-Damage);
    if (IsAlive()) return;
    DropBody(); Orders.Reset();
    if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
    const FVector Foot=Feet();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCapsuleHalfHeight(30);
    SetActorLocation(Foot+FVector(0,0,30));
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Visual->SetRelativeLocation(FVector(-75,0,-18));
    Visual->SetRelativeRotation(FRotator(90,0,0));
    Vision->bConeVisible=false; Vision->SetVisibility(false);
}
void AOKRTUnit::DropBody(bool bHide)
{
    auto* Body=CarriedBody.Get();
    if (!Body) return;
    Body->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Body->SetActorLocation(Feet()+GetActorRightVector()*65+FVector(0,0,30));
    Body->Carrier.Reset(); Body->bHiddenBody=bHide;
    Body->SetActorHiddenInGame(bHide);
    if (!bHide)
    {
        Body->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Body->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
        Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    }
    else Body->Stimuli->UnregisterFromPerceptionSystem();
    CarriedBody.Reset(); SetStance(Stance);
}
void AOKRTUnit::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!IsAlive()) return;
    auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!Game || Game->bWon || Game->bLost) return;
    Cooldown=FMath::Max(0.f,Cooldown-Delta);
    bInCover=(Stance==EOKStance::Crouch || Stance==EOKStance::Prone) &&
        Game->IsCover(Feet(),Stance==EOKStance::Prone ? 35.f : 95.f);
    if (!bEnemy) UpdateOrders();
    UpdateAnimation(Delta);
    NoiseClock+=Delta;
    if (!bEnemy && GetVelocity().Size2D()>20 && NoiseClock>.85f)
    {
        NoiseClock=0;
        const float Range=Stance==EOKStance::Run ? 1100 : Stance==EOKStance::Walk ? 260 : Stance==EOKStance::Crouch ? 110 : 55;
        Game->Noise(this,Feet(),1,Range,TEXT("Footstep"));
    }
}
