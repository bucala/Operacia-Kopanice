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
void AOKRTGuardController::Perceived(AActor* Actor,FAIStimulus Stimulus)
{
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    auto* Source=Cast<AOKRTUnit>(Actor);
    if (!bBrainEnabled || !Unit || !Unit->bEnemy || !Unit->IsAlive() || !Source || Source->bEnemy ||
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
    if (AlertState==EOKAlert::Combat) return;
    AlertState=EOKAlert::Investigate; LastKnown=Location;
    Suspicion=FMath::Max(Suspicion,.3f); Wait=0;
    MoveToLocation(Location,65,false,true,true,false,nullptr,false);
}
void AOKRTGuardController::Alert(AOKRTUnit* Target,bool bBroadcast)
{
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    if (!Unit || !Unit->IsAlive() || !Target || !Target->IsAlive()) return;
    const bool bNew=AlertState!=EOKAlert::Combat;
    AlertState=EOKAlert::Combat; Suspicion=1; CombatTarget=Target; LastKnown=Target->Feet();
    LostSight=0;
    if (bNew) ShotClock=-.65f;
    if (bNew && bBroadcast)
        if (auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>()) Game->Alarm(Target,Unit);
}
void AOKRTGuardController::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* Unit=Cast<AOKRTUnit>(GetPawn());
    auto* Game=GetWorld()->GetAuthGameMode<AOKRTGameMode>();
    if (!bBrainEnabled || !Unit || !Unit->bEnemy || !Unit->IsAlive() || !Game || Game->bWon || Game->bLost) return;
    MoveRetry=FMath::Max(0.f,MoveRetry-Delta);
    bool Seen=false;
    AOKRTUnit* SeenUnit=nullptr;
    float DetectionRate=0;
    bool Instant=false;
    for (AOKRTUnit* Party:Game->Party)
    {
        bool Primary=false;
        if (!Party->IsAlive() || !Unit->Vision->Sees(Party,Primary)) continue;
        const float Factor=Party->VisibilityFactor();
        const float Rate=(Primary ? 2.4f : .4f)*Factor;
        Seen=true;
        if (!SeenUnit || Rate>DetectionRate) { DetectionRate=Rate; SeenUnit=Party; }
        if (Primary && Factor>=.9f) { Instant=true; SeenUnit=Party; break; }
    }
    if (SeenUnit)
    {
        LastKnown=SeenUnit->Feet();
        Suspicion=Instant ? 1.f : FMath::Min(1.f,Suspicion+Delta*DetectionRate);
        if (Suspicion>=1) Alert(SeenUnit);
        else if (Suspicion>.5f && AlertState==EOKAlert::Patrol) Investigate(LastKnown);
    }
    for (AOKRTUnit* Body:Game->Enemies)
    {
        bool Primary=false;
        if (Body!=Unit && !Body->IsAlive() && !DiscoveredBodies.Contains(Body) && Unit->Vision->Sees(Body,Primary))
        { DiscoveredBodies.Add(Body); Investigate(Body->Feet()); }
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
            if (FVector::Dist2D(Unit->Feet(),Target->Feet())>650)
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
                if (ShotClock>1.1f)
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
            if (GetMoveStatus()==EPathFollowingStatus::Idle && MoveRetry<=0 && FVector::Dist2D(Unit->Feet(),LastKnown)>90)
            { MoveToLocation(LastKnown,70,false,true,true,false,nullptr,false); MoveRetry=.5f; }
        }
        if (LostSight>6 || !Target || !Target->IsAlive())
        { AlertState=EOKAlert::Investigate; CombatTarget.Reset(); PursuitTarget.Reset(); ClearFocus(EAIFocusPriority::Gameplay); Wait=0; }
        return;
    }
    if (GetMoveStatus()!=EPathFollowingStatus::Idle) return;
    Wait+=Delta;
    if (AlertState==EOKAlert::Investigate)
    {
        Unit->AddActorWorldRotation(FRotator(0,Delta*32,0));
        if (Wait<4) return;
        AlertState=EOKAlert::Patrol; Suspicion=.15f; Wait=0;
    }
    if (PatrolRoute.Num() && Wait>1)
    {
        Wait=0;
        PatrolIndex%=PatrolRoute.Num();
        MoveToLocation(PatrolRoute[PatrolIndex],25,false,true,true,false,nullptr,false);
        PatrolIndex=(PatrolIndex+1)%PatrolRoute.Num();
    }
}
