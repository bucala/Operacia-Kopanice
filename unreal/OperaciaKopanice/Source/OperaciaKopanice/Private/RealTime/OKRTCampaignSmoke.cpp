#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"

void AOKRTGameMode::CampaignSmokeTick()
{
    if (SmokeStage<0) return;
    const double Now=FPlatformTime::Seconds();
    static double LastReport=0;
    if (Now-LastReport>5)
    {
        LastReport=Now;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_CAMPAIGN_PROGRESS: mission=%d stage=%d queue=%d/%d feet=%s / %s objective=%d lost=%d message=%s"),
            MissionId,SmokeStage,Party[0]->QueueSize(),Party[1]->QueueSize(),*Party[0]->Feet().ToString(),*Party[1]->Feet().ToString(),bObjectiveComplete,bLost,*Message);
    }
    if (Now-SmokeStarted>180 || bLost) { Require(false,TEXT("campaign mission survives within timeout")); return; }
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* UI=CastChecked<AOKRTHUD>(PC->GetHUD());
    auto Next=[&]() { if (SmokeStage>=0) { ++SmokeStage; StageStarted=Now; } };
    auto Click=[&](FName Id)
    { FVector2D Point; Require(UI->FindButton(Id,Point) && UI->Click(Point),*FString::Printf(TEXT("campaign menu action %s"),*Id.ToString())); };
    auto Move=[](AOKRTUnit* Unit,FVector P,bool Append=true)
    { FOKRTOrder O; O.Location=P; return Unit->Submit(O,Append); };
    auto Approach=[](AOKRTUnit* Unit,FVector P,EOKInteraction Kind)
    { FOKRTOrder O; O.Kind=EOKOrder::Interact; O.Location=P; O.Interaction=Kind; O.bApproachInteraction=true; return Unit->Submit(O,true); };
    if (SmokeStage>=18)
        for (AOKRTUnit* Guard:Enemies)
            if (!Guard->IsAlive() || !CastChecked<AOKRTGuardController>(Guard->GetController())->bBrainEnabled)
            { Require(false,TEXT("campaign retains all live guard brains")); return; }
    switch (SmokeStage)
    {
    case 0:
        if (bInitialPausePending || Now-StageStarted<2) return;
        Require(MissionId>0 && bTacticalPause && Enemies.Num()==3,TEXT("additional mission starts paused with three patrols"));
        Require(HasCompletePath(Party[0]->Feet(),TNTLocation) && HasCompletePath(TNTLocation,ExitLocation),TEXT("new mission objectives and extraction are connected"));
        SmokeForestTests();
        if (SmokeStage<0) return;
        for (AOKRTUnit* Guard:Enemies)
            for (const auto& Stop:CastChecked<AOKRTGuardController>(Guard->GetController())->PatrolRoute)
                Require(FVector::Dist2D(Guard->Feet(),Stop.Location)<35 || HasCompletePath(Guard->Feet(),Stop.Location),TEXT("new mission patrol stop is navigable"));
        {
            int32 Alternate=0,Vans=0,Instances=0;
            for (TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
            {
                const UStaticMesh* Mesh=It->GetStaticMeshComponent()->GetStaticMesh();
                if (Mesh && Mesh->GetName()==TEXT("SM_OK_CabinAlt")) ++Alternate;
                if (Mesh && Mesh->GetName()==TEXT("SM_OK_CommandVan")) ++Vans;
            }
            for (TActorIterator<AActor> It(GetWorld());It;++It)
            {
                TArray<UHierarchicalInstancedStaticMeshComponent*> Forest; It->GetComponents(Forest);
                for (const auto* Part:Forest) if (!It->IsHidden() && Part->IsVisible()) Instances+=Part->GetInstanceCount();
            }
            Require(Alternate>=2 && Vans>=1 && Instances>200,TEXT("additional imported cabins vehicle and instanced forest are present"));
        }
        bFrontEnd=true; OpenMenuPage(EOKMenuPage::Pause); Next(); break;
    case 1:
        Require(UI->ValidateLayout(),TEXT("main menu responsive button layout")); Click(TEXT("Missions"));
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_CampaignMenu%d.png"),MissionId),false,false); Next(); break;
    case 2:
        Require(MenuPage==EOKMenuPage::Missions && UI->ValidateLayout(),TEXT("three-mission selection page layout"));
        if (Now-StageStarted<.5) return;
        Click(TEXT("Mission2"));
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_CampaignBriefing%d.png"),MissionId),false,false); Next(); break;
    case 3:
        { FVector2D P; Require(PreviewMission==2 && MenuPage==EOKMenuPage::Briefing && UI->FindButton(TEXT("Deploy"),P) && UI->ValidateLayout(),TEXT("mission selection opens deployable briefing")); }
        if (Now-StageStarted<.5) return;
        Click(TEXT("MenuBack")); Next(); break;
    case 4: Click(TEXT("MenuBack")); Next(); break;
    case 5: Click(TEXT("Options")); Next(); break;
    case 6:
        Require(UI->ValidateLayout() && bOptions,TEXT("graphics options responsive layout"));
        Click(TEXT("OptionsTab1")); Next(); break;
    case 7:
        Click(TEXT("SensitivityUp")); Require(FMath::IsNearlyEqual(CameraSensitivity,1.25f),TEXT("camera sensitivity stepper changes setting"));
        Click(TEXT("SensitivityDown")); Click(TEXT("OptionsTab2")); Next(); break;
    case 8:
        Click(TEXT("Markers")); Require(!bObjectiveMarkers,TEXT("objective marker toggle changes setting"));
        Click(TEXT("Markers")); Click(TEXT("MenuBack")); Next(); break;
    case 9:
        bFrontEnd=false; OpenMenuPage(EOKMenuPage::Pause); Next(); break;
    case 10: Click(TEXT("Restart")); Next(); break;
    case 11:
        Require(MenuPage==EOKMenuPage::ConfirmRestart && UI->ValidateLayout(),TEXT("restart requires confirmation"));
        Click(TEXT("MenuBack")); Next(); break;
    case 12: Click(TEXT("Quit")); Next(); break;
    case 13:
        Require(MenuPage==EOKMenuPage::ConfirmQuit && UI->ValidateLayout(),TEXT("quit requires confirmation"));
        Click(TEXT("MenuBack")); Next(); break;
    case 14: Click(TEXT("Resume")); Next(); break;
    case 15:
        Require(!bMenu && bTacticalPause && UGameplayStatics::IsGamePaused(GetWorld()),TEXT("menu resume preserves planning pause"));
        OpenMenuPage(EOKMenuPage::Options);
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_CampaignOptions%d.png"),MissionId),false,false); Next(); break;
    case 16:
        Require(UI->ValidateLayout(),TEXT("interface options responsive layout"));
        if (Now-StageStarted<.5) return;
        MenuBack(); Next(); break;
    case 17:
        MenuBack(); TogglePause(); Select(0);
        Require(Approach(Party[0],TNTLocation,MissionId==1 ? EOKInteraction::CollectDocuments : EOKInteraction::CollectTNT),TEXT("campaign accepts contextual supply approach"));
        Party[0]->CancelOrders();
        {
            FOKRTOrder Crouch; Crouch.Kind=EOKOrder::Stance; Crouch.Stance=EOKStance::Crouch;
            FVector Shrub=Mission().Spawn+FVector(-180,-260,0); Shrub.Z=0;
            Require(Party[0]->Submit(Crouch) && Move(Party[0],Shrub),TEXT("campaign queues a crouched approach into live shrub cover"));
        }
        SmokeStage=23; StageStarted=Now; break;
    case 23:
        if (Party[0]->QueueSize()) return;
        Require(Party[0]->bInCover && UI->ValidateLayout(),TEXT("native movement reaches shrub cover with a responsive cover badge"));
        FocusSelected();
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_ForestCover%d.png"),MissionId),false,false);
        SmokeStage=24; StageStarted=Now; break;
    case 24:
        if (Now-StageStarted<.5) return;
        {
            FOKRTOrder Walk; Walk.Kind=EOKOrder::Stance; Walk.Stance=EOKStance::Walk;
            Require(Party[0]->Submit(Walk),TEXT("campaign can leave concealment and resume walking"));
        }
        // The western bypass avoids the central guard. Commands, not teleports,
        // move the team; both meshes and all enemy brains remain in production mode.
        if (MissionId==1) Require(Move(Party[0],FVector(900,2200,0)) && Approach(Party[0],TNTLocation,EOKInteraction::CollectDocuments),TEXT("courier queues western approach and document pickup"));
        else Require(Approach(Party[0],TNTLocation,EOKInteraction::CollectTNT),TEXT("command post queues TNT pickup"));
        Require(Move(Party[1],FVector(900,MissionId==1 ? 2200 : 2300,0)),TEXT("companion accepts western bypass"));
        FocusObjective();
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_CampaignScene%d.png"),MissionId),false,false);
        SmokeStage=18; StageStarted=Now; break;
    case 18:
        if (Party[0]->QueueSize() || Party[1]->QueueSize()) return;
        Require(MissionId==1 ? bObjectiveComplete : bHasTNT,TEXT("campaign supplies acquired using navigation and interaction"));
        Require(UI->ValidateLayout(),TEXT("additional mission gameplay HUD layout"));
        Click(TEXT("FocusObjective"));
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_CampaignObjective%d.png"),MissionId),false,false);
        if (MissionId==2)
        {
            Require(Move(Party[0],FVector(1300,2300,0)) && Approach(Party[0],DetonatorLocation,EOKInteraction::SabotageCommandPost),TEXT("command post queues approach and sabotage"));
            Require(Move(Party[1],FVector(3600,2600,0)),TEXT("companion moves to sabotage staging area"));
        }
        Next(); break;
    case 19:
        if (Party[0]->QueueSize() || Party[1]->QueueSize()) return;
        if (MissionId==2)
        {
            MissionAwayTime+=GetWorld()->GetDeltaSeconds();
            if (MissionAwayTime<.4f) return;
            Require(Enemies.ContainsByPredicate([](const auto& Guard)
                { return CastChecked<AOKRTGuardController>(Guard->GetController())->AlertState==EOKAlert::Investigate; }),TEXT("command post sabotage draws an active patrol to the noise"));
        }
        Require(bObjectiveComplete && !bWon,TEXT("primary objective alone does not finish the mission"));
        for (int32 I=0;I<Party.Num();++I)
        {
            Require(Move(Party[I],FVector(900,700+I*80,0)) && Move(Party[I],ExitLocation+FVector(0,I*80-40,0)),TEXT("team queues extraction route"));
        }
        Next(); break;
    case 20:
        if (!bWon) return;
        Require(MissionCompleted(MissionId) && Party[0]->Health==100 && Party[1]->Health==100,TEXT("additional mission completed with both units undamaged"));
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_CampaignWin%d.png"),MissionId),false,false);
        Next(); break;
    case 21:
        Require(UI->ValidateLayout(),TEXT("campaign results responsive layout"));
        if (MissionId==1) { Click(TEXT("NextMission")); Next(); }
        else
        {
            UE_LOG(LogTemp,Display,TEXT("OK_RT_CAMPAIGN: PASS mission=%d"),MissionId);
            FPlatformMisc::RequestExit(false); bCampaignSmoke=false; bSmoke=false;
        }
        break;
    case 22:
        Require(MenuPage==EOKMenuPage::Briefing && PreviewMission==2,TEXT("results offer the next mission briefing"));
        UE_LOG(LogTemp,Display,TEXT("OK_RT_CAMPAIGN: PASS mission=%d; travelling to next operation"),MissionId);
        Click(TEXT("Deploy")); Next(); break;
    }
}
