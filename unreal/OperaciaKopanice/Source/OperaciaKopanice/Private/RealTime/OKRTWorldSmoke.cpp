#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"

void AOKRTGameMode::SmokeWorldTests()
{
    for (int32 Id=0;Id<OKMissions::Count;++Id)
    {
        const auto& Definition=OKMissions::Get(Id);
        FOKRTMissionState State;
        Require(!OKMissions::CanExtract(State),TEXT("empty mission world can never report success"));
        State.PartyCount=State.AliveCount=2; State.ExitedCount=2;
        Require(!OKMissions::CanExtract(State),TEXT("extraction requires completion of the mission target"));
        const bool Documents=Definition.Goal==EOKMissionGoal::Documents;
        Require(OKMissions::Stage(Definition,State)==(Documents ? EOKObjectiveStage::Target : EOKObjectiveStage::Supply),
            TEXT("initial world objective matches authored mission type"));
        Require(OKMissions::CanExecute(Definition,State,EOKInteraction::CollectTNT)!=Documents &&
            OKMissions::CanExecute(Definition,State,EOKInteraction::CollectDocuments)==Documents,
            TEXT("mission world cannot execute another operation's supply interaction"));
        State.bSupplies=true;
        if (Definition.Goal==EOKMissionGoal::Bridge)
        {
            State.SafeBankCount=1;
            Require(OKMissions::Stage(Definition,State)==EOKObjectiveStage::Crossing &&
                !OKMissions::CanExecute(Definition,State,EOKInteraction::DetonateBridge),
                TEXT("bridge world rejects sabotage until every member clears the span"));
            State.SafeBankCount=2;
            Require(OKMissions::Stage(Definition,State)==EOKObjectiveStage::Target &&
                OKMissions::CanExecute(Definition,State,EOKInteraction::DetonateBridge),
                TEXT("safe bank unlocks the bridge target without turn execution"));
        }
        if (Definition.Goal==EOKMissionGoal::CommandPost)
            Require(OKMissions::CanExecute(Definition,State,EOKInteraction::SabotageCommandPost),
                TEXT("command post target requires collected supplies"));
        State.bTargetComplete=true; State.ExitedCount=1;
        Require(OKMissions::Stage(Definition,State)==EOKObjectiveStage::Extraction && !OKMissions::CanExtract(State),
            TEXT("completed target advances the objective but a partial team cannot extract"));
        State.ExitedCount=2;
        Require(OKMissions::CanExtract(State),TEXT("complete living party can extract after the objective"));
        State.AliveCount=1;
        Require(!OKMissions::CanExtract(State) && !OKMissions::CanExecute(Definition,State,EOKInteraction::CollectTNT),
            TEXT("casualty blocks success and objective side effects"));
        Require(!Definition.Guards.IsEmpty(),TEXT("each mission declares an independent guard population"));
        for (const auto& Guard:Definition.Guards)
            Require(!Guard.Route.IsEmpty() && FVector::Dist2D(Guard.Spawn,Guard.Route[0].Location)<1 &&
                (Guard.Duty!=EOKGuardDuty::HoldPost || Guard.Route.Num()==1),
                TEXT("authored guard has a spawn watch and a coherent patrol duty"));
    }
    Require(Enemies.Num()==Mission().Guards.Num(),TEXT("scene guard population comes from mission data"));
    for (int32 I=0;I<Enemies.Num();++I)
    {
        const auto* AI=CastChecked<AOKRTGuardController>(Enemies[I]->GetController());
        Require(AI->Duty==Mission().Guards[I].Duty && AI->PatrolRoute.Num()==Mission().Guards[I].Route.Num(),
            TEXT("live controllers use the authored duty and patrol route"));
    }

    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Guard=GetWorld()->SpawnActor<AOKRTUnit>(FVector(720,720,90),FRotator(0,90,0),Params);
    auto* Target=GetWorld()->SpawnActor<AOKRTUnit>(FVector(720,1320,90),FRotator::ZeroRotator,Params);
    Guard->Initialize(true,true); Target->Initialize(false,false);
    auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController()); AI->bBrainEnabled=false;
    bool Instant=false;
    Require(AI->DetectionRate(Target,Instant)>.39f && !Instant,TEXT("far standing unit accumulates suspicion rather than instant detection"));
    Target->SetStance(EOKStance::Crouch);
    Require(AI->DetectionRate(Target,Instant)>.13f && AI->DetectionRate(Target,Instant)<.15f,
        TEXT("far crouching unit remains detectable at a slower rate"));
    Target->SetStance(EOKStance::Prone);
    Require(AI->DetectionRate(Target,Instant)==0 && !Instant,TEXT("far cone cannot visually detect a crawling unit"));
    Target->SetActorLocation(FVector(720,930,90));
    Require(AI->DetectionRate(Target,Instant)>0,TEXT("near cone detects a crawling unit"));
    Target->SetStance(EOKStance::Walk);
    Require(AI->DetectionRate(Target,Instant)>2 && Instant,TEXT("near standing exposure confirms detection immediately"));
    Target->SetActorLocation(FVector(450,270,90));
    Guard->SetActorRotation((Target->Feet()-Guard->Feet()).Rotation());
    Require(AI->DetectionRate(Target,Instant)==0,TEXT("world collision blocks all visual detection zones"));
    Target->SetActorLocation(FVector(720,1320,90)); Guard->SetActorRotation(FRotator(0,90,0));
    AI->Duty=EOKGuardDuty::HoldPost;
    AI->Investigate(Target->Feet());
    Require(AI->AlertState==EOKAlert::Investigate && AI->GetMoveStatus()==EPathFollowingStatus::Idle,
        TEXT("fixed sentry reacts to a disturbance without abandoning its post"));
    AI->Alert(Target,false); AI->bBrainEnabled=true; AI->Tick(.016f);
    Require(AI->AlertState==EOKAlert::Combat && AI->GetMoveStatus()==EPathFollowingStatus::Idle,
        TEXT("fixed sentry can raise combat awareness without chasing"));
    AI->bBrainEnabled=false; AI->AlertState=EOKAlert::Patrol; AI->Duty=EOKGuardDuty::Patrol;
    AI->Investigate(Target->Feet());
    Require(AI->GetMoveStatus()!=EPathFollowingStatus::Idle,TEXT("mobile patrol investigates along a NavMesh path"));
    AI->StopMovement(); AI->Destroy();
    Target->GetController()->Destroy(); Target->Destroy(); Guard->Destroy();
}
