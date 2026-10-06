#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTPlayerController.h"
#include "Engine/World.h"
#include "UnrealClient.h"

// Exercises the authored mission using player orders only. Guards keep their normal
// perception, routes and damage throughout; no teleporting, takedowns or invulnerability.
void AOKRTGameMode::MissionSmokeTick(float Delta)
{
    if (SmokeStage<0) return;
    if (FPlatformTime::Seconds()-SmokeStarted>200)
    { Require(false,TEXT("live mission timeout")); return; }
    if (bLost) { Require(false,TEXT("live mission party survives")); return; }
    const double Now=GetWorld()->GetTimeSeconds();
    static double LastReport=-5;
    if (Now-LastReport>=5)
    {
        LastReport=Now;
        for (AOKRTUnit* Guard:Enemies)
        {
            auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController());
            UE_LOG(LogTemp,Display,TEXT("OK_RT_MISSION_PATROL: t=%.2f stage=%d pos=%s yaw=%.1f state=%d suspicion=%.3f party=%s / %s"),
                Now,SmokeStage,*Guard->Feet().ToString(),Guard->GetActorRotation().Yaw,int32(AI->AlertState),AI->Suspicion,
                *Party[0]->Feet().ToString(),*Party[1]->Feet().ToString());
        }
    }
    auto Next=[this,Now]()
    {
        if (SmokeStage<0) return;
        ++SmokeStage; StageStarted=Now;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_MISSION_STAGE: %d at %.2f seconds"),SmokeStage,Now);
    };
    auto Move=[](AOKRTUnit* Unit,FVector Location,bool Append=false)
    { FOKRTOrder Order; Order.Location=Location; Unit->Submit(Order,Append); };
    auto Interact=[](AOKRTUnit* Unit)
    { FOKRTOrder Order; Order.Kind=EOKOrder::Interact; Unit->Submit(Order,true); };
    auto* West=CastChecked<AOKRTGuardController>(Enemies[0]->GetController());
    for (AOKRTUnit* Guard:Enemies)
    {
        auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController());
        if (SmokeStage>0 && (!AI->bBrainEnabled || !Guard->IsAlive() || AI->AlertState==EOKAlert::Combat))
        {
            UE_LOG(LogTemp,Error,TEXT("OK_RT_MISSION_ALERT: guard=%s yaw=%.1f party=%s / %s"),
                *Guard->Feet().ToString(),Guard->GetActorRotation().Yaw,*Party[0]->Feet().ToString(),*Party[1]->Feet().ToString());
            Require(false,TEXT("live stealth route avoids combat with both guards active")); return;
        }
    }
    switch (SmokeStage)
    {
    case 0:
        if (bInitialPausePending) return;
        Require(bTacticalPause,TEXT("mission starts in planning pause"));
        for (AOKRTUnit* Guard:Enemies)
        {
            auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController());
            for (const auto& Stop:AI->PatrolRoute)
                Require(FVector::Dist2D(Guard->Feet(),Stop.Location)<35 || HasCompletePath(Guard->Feet(),Stop.Location),
                    TEXT("authored patrol stop is reachable"));
        }
        TogglePause(); Next(); break;
    case 1:
        for (AOKRTUnit* Guard:Enemies)
        {
            auto* AI=CastChecked<AOKRTGuardController>(Guard->GetController());
            if (AI->AlertState!=EOKAlert::Patrol || AI->Suspicion>.01f)
            {
                UE_LOG(LogTemp,Error,TEXT("OK_RT_MISSION_SPAWN: pos=%s yaw=%.1f state=%d suspicion=%.3f"),
                    *Guard->Feet().ToString(),Guard->GetActorRotation().Yaw,int32(AI->AlertState),AI->Suspicion);
                Require(false,TEXT("idle spawn stays outside patrol sight over two cycles")); return;
            }
        }
        {
            const FVector ToBridge=(FVector(1080,720,0)-Enemies[0]->Feet()).GetSafeNormal2D();
            const bool Watches=FVector::DotProduct(Enemies[0]->GetActorForwardVector(),ToBridge)>
                FMath::Cos(FMath::DegreesToRadians(Enemies[0]->Vision->HalfAngle));
            bMissionSawBridge|=Watches;
            MissionAwayTime=Watches ? 0 : MissionAwayTime+Delta;
            MissionLongestAway=FMath::Max(MissionLongestAway,MissionAwayTime);
        }
        if (Now-StageStarted<40) return;
        Require(bMissionSawBridge && MissionLongestAway>=10,TEXT("bridge has repeated watch periods and ten-second crossing windows"));
        Require(true,TEXT("idle spawn remains safe for forty real-time seconds"));
        for (AOKRTUnit* Unit:Party)
        { FOKRTOrder Order; Order.Kind=EOKOrder::Stance; Order.Stance=EOKStance::Crouch; Unit->Submit(Order); }
        {
            auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
            FVector2D Target;
            Select(0);
            Require(PC->ProjectWorldLocationToScreen(TNTLocation,Target) && PC->CommandAt(Target),
                TEXT("live mission accepts contextual TNT approach from screen targeting"));
            if (SmokeStage<0) return;
        }
        Move(Party[0],FVector(900,950,0),true);
        Move(Party[1],FVector(820,950,0));
        Next(); break;
    case 2:
        if (Party[0]->QueueSize() || Party[1]->QueueSize()) return;
        Require(bHasTNT && FVector::Dist2D(Party[0]->Feet(),FVector(900,950,0))<65,
            TEXT("TNT pickup and concealed bridge approach with live patrols"));
        Next(); break;
    case 3:
        // Start crossing when the west guard has returned to his long north-facing stop.
        if (West->AlertState!=EOKAlert::Patrol || FVector::Dist2D(Enemies[0]->Feet(),FVector(660,700,0))>35 ||
            FMath::Abs(FMath::FindDeltaAngleDegrees(Enemies[0]->GetActorRotation().Yaw,-90.f))>3 ||
            West->PatrolWaitRemaining()<10) return;
        Move(Party[0],FVector(1300,710,0)); Move(Party[1],FVector(1300,790,0));
        Next(); break;
    case 4:
        if (Party[0]->QueueSize() || Party[1]->QueueSize()) return;
        Require(Party[0]->Feet().X>1195 && Party[1]->Feet().X>1195,TEXT("both units cross during patrol window without combat"));
        Move(Party[0],DetonatorLocation+FVector(-50,-40,0));
        Move(Party[1],FVector(1330,560,0));
        Next(); break;
    case 5:
        if (Party[0]->QueueSize() || Party[1]->QueueSize()) return;
        Interact(Party[0]); Next(); break;
    case 6:
        if (Party[0]->QueueSize()) return;
        Require(bBridgeDestroyed,TEXT("live mission sabotage"));
        for (AOKRTUnit* Unit:Party)
        { FOKRTOrder Order; Order.Kind=EOKOrder::Stance; Order.Stance=EOKStance::Walk; Unit->Submit(Order); }
        Move(Party[0],ExitLocation+FVector(0,-40,0)); Move(Party[1],ExitLocation+FVector(0,40,0));
        Next(); break;
    case 7:
        if (!bWon) return;
        Require(Party[0]->Health==100 && Party[1]->Health==100,TEXT("live mission extraction without damage or killed guards"));
        Require(!HasCompletePath(FVector(720,720,0),DetonatorLocation),TEXT("live sabotage removes bridge route"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_MissionWin.png"),false,false);
        Next(); StageStarted=FPlatformTime::Seconds(); break;
    case 8:
        if (FPlatformTime::Seconds()-StageStarted<1) return;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_MISSION: PASS"));
        FPlatformMisc::RequestExit(false); bMissionSmoke=false; bSmoke=false; break;
    }
}
