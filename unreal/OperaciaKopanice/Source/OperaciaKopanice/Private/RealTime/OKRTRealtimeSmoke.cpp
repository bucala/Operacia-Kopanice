#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"

void AOKRTGameMode::RealtimeSmokeTick()
{
    if (SmokeStage<0 || bInitialPausePending) return;
    const double Now=FPlatformTime::Seconds();
    if (Now-SmokeStarted>65) { Require(false,TEXT("realtime startup fixture timeout")); return; }
    auto Next=[&]() { ++SmokeStage; StageStarted=Now; };
    switch (SmokeStage)
    {
    case 0:
        Require(!bTacticalPause && !bMenu && !GetWorld()->IsPaused(),TEXT("production deployment starts running without a Space press"));
        Select(0); PausePosition=Party[0]->Feet();
        { FOKRTOrder O; O.Location=FVector(720,1260,0); Require(Command(O),TEXT("movement accepted immediately without advancing a turn")); }
        Next(); break;
    case 1:
        if (Now-StageStarted<3) return;
        Require(FVector::Dist2D(PausePosition,Party[0]->Feet())>120,TEXT("character moves continuously after one destination command"));
        Require(!bTacticalPause && !GetWorld()->IsPaused(),TEXT("movement never automatically pauses the simulation"));
        Next(); break;
    case 2:
        bRealtimeSawPatrol|=Enemies.ContainsByPredicate([](const auto& U) { return U->GetVelocity().Size2D()>5; });
        if (Now-StageStarted<14) return;
        Require(bRealtimeSawPatrol,TEXT("guards patrol while the player issues no further input"));
        Require(!bLost,TEXT("initial realtime patrol window remains survivable"));
        TogglePause(); PausePosition=Party[0]->Feet(); Next(); break;
    case 3:
        if (Now-StageStarted<1) return;
        Require(GetWorld()->IsPaused() && Party[0]->Feet().Equals(PausePosition,1),TEXT("optional tactical pause freezes movement"));
        { FOKRTOrder O; O.Location=FVector(810,1260,0); Require(Command(O),TEXT("tactical pause accepts a queued command")); }
        OpenMenuPage(EOKMenuPage::Pause); MenuBack();
        Require(bTacticalPause && GetWorld()->IsPaused(),TEXT("leaving menu preserves an explicitly requested tactical pause"));
        TogglePause(); PausePosition=Party[0]->Feet(); Next(); break;
    case 4:
        if (Now-StageStarted<2) return;
        Require(!GetWorld()->IsPaused() && FVector::Dist2D(PausePosition,Party[0]->Feet())>35,TEXT("resuming executes queued movement and stays realtime"));
        OpenMenuPage(EOKMenuPage::Pause); MenuBack();
        Require(!bTacticalPause && !GetWorld()->IsPaused(),TEXT("returning from menu resumes a running mission"));
        FScreenshotRequest::RequestScreenshot(TEXT("OKRT_RealTimeRunning.png"),false,false); Next(); break;
    case 5:
        if (Now-StageStarted<.5) return;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_REALTIME: PASS"));
        FPlatformMisc::RequestExit(false); bRealtimeSmoke=false; bSmoke=false; break;
    }
}
