#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTVisionComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"

AOKRTGuardController::AOKRTGuardController()
{
    PrimaryActorTick.bCanEverTick=true;
    Senses=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("RealtimePerception"));
    SetPerceptionComponent(*Senses);
    auto* Sight=CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
    Sight->SightRadius=1100; Sight->LoseSightRadius=1200; Sight->PeripheralVisionAngleDegrees=45;
    Sight->DetectionByAffiliation.bDetectEnemies=true;
    Sight->DetectionByAffiliation.bDetectFriendlies=true;
    Sight->DetectionByAffiliation.bDetectNeutrals=true;
    Sight->SetMaxAge(4);
    auto* Hearing=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
    Hearing->HearingRange=1600;
    Hearing->DetectionByAffiliation=Sight->DetectionByAffiliation;
    Hearing->SetMaxAge(4);
    Senses->ConfigureSense(*Sight); Senses->ConfigureSense(*Hearing);
    Senses->SetDominantSense(UAISense_Sight::StaticClass());
    Senses->OnTargetPerceptionUpdated.AddDynamic(this,&AOKRTGuardController::Perceived);
}
FGenericTeamId AOKRTGuardController::GetGenericTeamId() const
{
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    return FGenericTeamId(Unit && Unit->bEnemy ? 2 : 1);
}
float AOKRTGuardController::PatrolWaitRemaining() const
{
    return AlertState==EOKAlert::Patrol && bAtPatrolStop && PatrolRoute.IsValidIndex(PatrolIndex) ?
        FMath::Max(0.f,PatrolRoute[PatrolIndex].WaitSeconds-Wait) : 0.f;
}
void AOKRTGuardController::Perceived(AActor* Actor,FAIStimulus Stimulus)
{
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    auto* Source=Cast<AOKRTUnit>(Actor);
    if (!bBrainEnabled || !Unit || !Unit->bEnemy || !Unit->IsOperational() || !Source || Source->bEnemy ||
        !Stimulus.WasSuccessfullySensed() || Stimulus.Type!=UAISense::GetSenseID<UAISense_Hearing>()) return;
    // Hearing events carry an explicit max range. Walls further reduce audibility.
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRTHearing),false,Unit);
    Params.AddIgnoredActor(Source);
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Unit->GetActorLocation(),Stimulus.StimulusLocation+FVector(0,0,90),ECC_Visibility,Params);
    if (!Blocked || FVector::Dist2D(Unit->Feet(),Stimulus.StimulusLocation)<500)
        Investigate(Stimulus.StimulusLocation);
}
void AOKRTGuardController::Investigate(FVector Location)
{
    const auto* Unit=Cast<AOKRTUnit>(GetPawn());
    if (!Unit || !Unit->IsOperational()) return;
    if (AlertState==EOKAlert::Combat) return;
    RescueTarget.Reset();
    AlertState=EOKAlert::Investigate; LastKnown=Location;
    bAtPatrolStop=false;
    Suspicion=FMath::Max(Suspicion,.3f); Wait=0;
    if (Duty==EOKGuardDuty::HoldPost) StopMovement();
    else MoveToLocation(Location,65,false,true,true,false,nullptr,false);
}
void AOKRTGuardController::Alert(AOKRTUnit* Target,bool bBroadcast)
{
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    if (!Unit || !Unit->IsOperational() || !Target || !Target->IsOperational()) return;
    RescueTarget.Reset();
    const bool bNew=AlertState!=EOKAlert::Combat;
    AlertState=EOKAlert::Combat; Suspicion=1; CombatTarget=Target; LastKnown=Target->Feet();
    bAtPatrolStop=false;
    LostSight=0;
    if (bNew) ShotClock=-.65f;
    if (bNew && bBroadcast)
        if (auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>()) Game->Alarm(Target,Unit);
}
float AOKRTGuardController::DetectionRate(const AOKRTUnit* Target,bool& bInstant) const
{
    bInstant=false;
    const auto* Unit=Cast<AOKRTUnit>(GetPawn());
    bool Primary=false;
    if (!Unit || !Target || !Target->IsAlive() || !Unit->Vision->Sees(Target,Primary)) return 0;
    if (!Primary && Target->Stance==EOKStance::Prone) return 0;
    const float Factor=Target->VisibilityFactor();
    bInstant=Primary && Factor>=.9f;
    return (Primary ? 2.4f : .4f)*Factor;
}
void AOKRTGuardController::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!bBrainEnabled || !Unit || !Unit->bEnemy || !Unit->IsOperational() || !Game || Game->bWon || Game->bLost || Game->bTacticalPause || Game->bMenu) return;
    auto RescuePath=[Unit](AOKRTUnit* Body)
    {
        FOKRTOrder Order; Order.Kind=EOKOrder::Bind; Order.Target=Body;
        TArray<FVector> Points; FString Reason;
        return Unit->GetOrderPath(Order,false,Points,Reason);
    };
    MoveRetry=FMath::Max(0.f,MoveRetry-Delta);
    bool Seen=false;
    AOKRTUnit* SeenUnit=nullptr;
    float BestDetectionRate=0;
    bool Instant=false;
    for (AOKRTUnit* Party:Game->Party)
    {
        bool Immediate=false;
        const float Rate=DetectionRate(Party,Immediate);
        if (Rate<=0) continue;
        Seen=true;
        if (!SeenUnit || Rate>BestDetectionRate) { BestDetectionRate=Rate; SeenUnit=Party; }
        if (Immediate) { Instant=true; SeenUnit=Party; break; }
    }
    if (SeenUnit)
    {
        LastKnown=SeenUnit->Feet();
        Suspicion=Instant ? 1.f : FMath::Min(1.f,Suspicion+Delta*BestDetectionRate);
        if (Suspicion>=1) Alert(SeenUnit);
        else if (Suspicion>.5f && AlertState==EOKAlert::Patrol) Investigate(LastKnown);
    }
    for (AOKRTUnit* Body:Game->Enemies)
    {
        bool Primary=false;
        if (Body->IsOperational()) DiscoveredBodies.Remove(Body);
        if (Body!=Unit && Body->IsBody() && !DiscoveredBodies.Contains(Body) && Unit->Vision->Sees(Body,Primary))
        {
            DiscoveredBodies.Add(Body);
            if (AlertState==EOKAlert::Combat) continue;
            Investigate(Body->Feet());
            // Fixed sentries keep their post. Mobile guards remember an observed
            // casualty, but still need a complete route and physical reach.
            if (Body->bUnconscious && Duty==EOKGuardDuty::Patrol && RescuePath(Body))
                RescueTarget=Body;
        }
    }
    if (!Seen && AlertState!=EOKAlert::Combat) Suspicion=FMath::Max(0.f,Suspicion-Delta*.09f);
    if (AlertState==EOKAlert::Combat)
    {
        LostSight=Seen ? 0 : LostSight+Delta;
        auto* Target=SeenUnit ? SeenUnit : CombatTarget.Get();
        if (Seen && Target)
        {
            SetFocus(Target);
            const FVector Direction=(Target->Feet()-Unit->Feet()).GetSafeNormal2D();
            Unit->SetActorRotation(Direction.Rotation());
            if (FVector::Dist2D(Unit->Feet(),Target->Feet())>650 && Duty!=EOKGuardDuty::HoldPost)
            {
                // Actor goals already track motion; do not abort/recreate the path every frame.
                if (PursuitTarget.Get()!=Target || (GetMoveStatus()==EPathFollowingStatus::Idle && MoveRetry<=0))
                {
                    MoveToActor(Target,450,false,true,true,nullptr,false);
                    PursuitTarget=Target; MoveRetry=.5f;
                }
            }
            else
            {
                if (GetMoveStatus()!=EPathFollowingStatus::Idle) StopMovement();
                PursuitTarget.Reset(); ShotClock+=Delta;
                if (ShotClock>1.1f && FVector::Dist2D(Unit->Feet(),Target->Feet())<=650)
                {
                    ShotClock=0; Target->TakeHit(35);
                    Game->Noise(Unit,Unit->Feet(),1,1600,TEXT("Gunshot"));
                }
            }
        }
        else
        {
            ClearFocus(EAIFocusPriority::Gameplay);
            // Losing sight must replace the actor goal, otherwise AI follows an unseen player.
            if (PursuitTarget.IsValid()) { StopMovement(); PursuitTarget.Reset(); MoveRetry=0; }
            if (Duty!=EOKGuardDuty::HoldPost && GetMoveStatus()==EPathFollowingStatus::Idle && MoveRetry<=0 && FVector::Dist2D(Unit->Feet(),LastKnown)>90)
            { MoveToLocation(LastKnown,70,false,true,true,false,nullptr,false); MoveRetry=.5f; }
        }
        if (LostSight>6 || !Target || !Target->IsAlive())
        { AlertState=EOKAlert::Investigate; CombatTarget.Reset(); PursuitTarget.Reset(); ClearFocus(EAIFocusPriority::Gameplay); Wait=0; }
        return;
    }
    if (auto* Body=RescueTarget.Get())
    {
        if (!Body->bUnconscious || !Body->IsAlive() || Body->bHiddenBody || Body->Carrier.IsValid() || Duty!=EOKGuardDuty::Patrol)
        { StopMovement(); RescueTarget.Reset(); Wait=0; }
        else
        {
            LastKnown=Body->Feet();
            if (GetMoveStatus()!=EPathFollowingStatus::Idle) return;
            if (FVector::Dist2D(Unit->Feet(),Body->Feet())>145)
            {
                if (MoveRetry<=0)
                {
                    MoveRetry=.5f;
                    if (!RescuePath(Body) ||
                        MoveToActor(Body,100,false,true,true,nullptr,false)==EPathFollowingRequestResult::Failed)
                        RescueTarget.Reset();
                }
                return;
            }
            FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRescue),false,Unit); Params.AddIgnoredActor(Body);
            const bool Blocked=FMath::Abs(Unit->Feet().Z-Body->Feet().Z)>75 ||
                GetWorld()->LineTraceSingleByChannel(Hit,Unit->GetActorLocation(),Body->GetActorLocation(),ECC_Visibility,Params);
            if (!Blocked && Body->WakeUp()) Game->Message=TEXT("Hliadka oslobodila omraceneho spolubojovnika.");
            RescueTarget.Reset(); Wait=0;
        }
    }
    if (GetMoveStatus()!=EPathFollowingStatus::Idle) return;
    if (AlertState==EOKAlert::Investigate)
    {
        Wait+=Delta;
        if (Duty==EOKGuardDuty::HoldPost)
            Unit->SetActorRotation(FMath::RInterpConstantTo(Unit->GetActorRotation(),FRotator(0,(LastKnown-Unit->Feet()).Rotation().Yaw,0),Delta,90));
        else Unit->AddActorWorldRotation(FRotator(0,Delta*32,0));
        if (Wait<4) return;
        AlertState=EOKAlert::Patrol; Suspicion=.15f; Wait=0;
    }
    if (PatrolRoute.IsEmpty()) return;
    PatrolIndex%=PatrolRoute.Num();
    const FOKRTPatrolStop& Stop=PatrolRoute[PatrolIndex];
    if (FVector::Dist2D(Unit->Feet(),Stop.Location)>35)
    {
        bAtPatrolStop=false; Wait=0;
        // A failed/aborted path is retried, never counted as arrival at a watch post.
        if (MoveRetry<=0)
        { MoveToLocation(Stop.Location,20,false,true,true,false,nullptr,false); MoveRetry=1; }
        return;
    }
    if (!bAtPatrolStop) { bAtPatrolStop=true; Wait=0; }
    const FRotator Facing(0,Stop.FacingYaw,0);
    Unit->SetActorRotation(FMath::RInterpConstantTo(Unit->GetActorRotation(),Facing,Delta,90));
    // Observation time starts only after the visible turn has finished.
    if (FMath::Abs(FMath::FindDeltaAngleDegrees(Unit->GetActorRotation().Yaw,Stop.FacingYaw))>2) return;
    Wait+=Delta;
    if (Wait>=FMath::Max(0.f,Stop.WaitSeconds))
    { PatrolIndex=(PatrolIndex+1)%PatrolRoute.Num(); bAtPatrolStop=false; Wait=0; MoveRetry=0; }
}
