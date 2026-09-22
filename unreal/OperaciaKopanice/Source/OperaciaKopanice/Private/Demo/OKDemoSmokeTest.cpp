#include "Demo/OKDemoGameMode.h"
#include "Demo/OKDemoHUD.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Navigation/OKViaductActor.h"

void AOKDemoGameMode::SetupSmokeTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(),TEXT("OKDemoSmoke"))) return;
    bGridVisible=true; bCameraEnabled=true; bSteppedRotation=true;
    RefreshScene();
    auto Later=[this](float Delay,TFunction<void()> Action)
    {
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(this,MoveTemp(Action)),Delay,false);
    };
    Later(8.f,[] { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DemoStart.png"),true,false); });
    Later(14.f,[this]()
    {
        using namespace OKDemo;
        bool bPassed=bContinuousTerrain;
        auto CheckScene=[this]()
        {
            bool Valid=PlayerMarker->GetActorLocation().Equals(CellLocation(State.Player),.01);
            Valid &= Bridge->IsDemoModelConsistent() && Bridge->bDestroyed==State.bBridgeDestroyed;
            Valid &= Bridge->GetActorLocation().Equals(CellLocation({6,OKDemo::GetLevel(State.LevelIndex).BridgeRow}),.01);
            for (int32 I=0;I<Guards.Num();++I)
                Valid &= Guards[I]->GetActorLocation().Equals(CellLocation(GuardPosition(I,State.Turn,State.LevelIndex)),.01);
            for (int32 Y=0;Y<Height;++Y)
                for (int32 X=0;X<Width;++X)
                {
                    const FIntPoint Cell(X,Y);
                    auto* Tile=Tiles[Y*Width+X].Get();
                    Valid &= Tile->IsHidden()==IsBlocked(Cell,State.bBridgeDestroyed,State.LevelIndex);
                    auto* M=Cast<UMaterialInstanceDynamic>(Tile->GetStaticMeshComponent()->GetMaterial(0));
                    float Fill=-1,Grid=-1;
                    Valid &= M && M->GetScalarParameterValue(TEXT("Fill"),Fill) &&
                        M->GetScalarParameterValue(TEXT("GridVisibility"),Grid);
                    Valid &= FMath::IsNearlyEqual(Grid,bGridVisible ? 1.f : 0.f);
                    if (IsThreatened(Cell,State) && Cell!=State.Player && !Tile->IsHidden())
                        Valid &= FMath::IsNearlyEqual(Fill,.25f);
                }
            return Valid;
        };
        for (int32 L=0;L<LevelCount;++L)
        {
            SelectLevel(L);
            const auto Route=FindSolution(L);
            bool LevelPassed=!Route.IsEmpty() && CheckScene();
            for (auto Action : Route)
            {
                const bool WasDestroyed=State.bBridgeDestroyed;
                Act(Action);
                LevelPassed &= CheckScene();
                if (!WasDestroyed && State.bBridgeDestroyed)
                {
                    Undo();
                    LevelPassed &= !State.bBridgeDestroyed && CheckScene();
                    Act(Action);
                    LevelPassed &= State.bBridgeDestroyed && CheckScene();
                }
            }
            LevelPassed &= State.Outcome==EOutcome::Won && State.Turn==Route.Num();
            LevelPassed &= GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>()->TurnNumber==State.Turn;
            if (!Route.IsEmpty())
            {
                Undo(); LevelPassed &= State.Outcome==EOutcome::Playing && CheckScene();
                Act(Route.Last()); LevelPassed &= State.Outcome==EOutcome::Won;
            }
            Restart();
            LevelPassed &= History.IsEmpty() && State.Turn==0 && CheckScene();
            UE_LOG(LogTemp,Display,TEXT("OK_LEVEL_%d: %s (%d turns)"),L+1,LevelPassed ? TEXT("PASS") : TEXT("FAIL"),Route.Num());
            bPassed &= LevelPassed;
        }
        SelectLevel(0);
        const int32 Before=State.Turn;
        SetMenuPage(1); Act(EAction::Wait); Undo();
        bPassed &= State.Turn==Before;
        SetMenuPage(2); ToggleMenu(); bPassed &= MenuPage==1;
        ToggleMenu(); bPassed &= MenuPage==0;
        ToggleGrid(); bPassed &= CheckScene(); ToggleGrid(); bPassed &= CheckScene();
        const FVector Original=Camera->GetActorLocation();
        ToggleCamera(); Orbit(45,10,-180);
        bPassed &= Camera->GetActorLocation().Equals(Original,.01);
        ToggleCamera();
        auto* PC=GetWorld()->GetFirstPlayerController();
        int32 VW=0,VH=0; PC->GetViewportSize(VW,VH);
        bool CameraPassed=true;
        for (int32 Step=0;Step<8;++Step)
        {
            PC->PlayerCameraManager->UpdateCamera(0);
            for (int32 Y=0;Y<Height;++Y)
                for (int32 X=0;X<Width;++X)
                {
                    if (IsBlocked({X,Y},false)) continue;
                    FVector2D S; FVector Origin,Direction;
                    bool Valid=PC->ProjectWorldLocationToScreen(CellLocation({X,Y}),S);
                    Valid &= PC->DeprojectScreenPositionToWorld(S.X,S.Y,Origin,Direction);
                    Valid &= S.X>8 && S.X<VW-8 && S.Y>96 && S.Y<VH-80 && !FMath::IsNearlyZero(Direction.Z);
                    if (!FMath::IsNearlyZero(Direction.Z))
                    {
                        const FVector P=Origin-Direction*(Origin.Z/Direction.Z);
                        Valid &= FIntPoint(FMath::RoundToInt(P.X/CellSize),FMath::RoundToInt(P.Y/CellSize))==FIntPoint(X,Y);
                    }
                    CameraPassed &= Valid;
                }
            Orbit(45,0);
        }
        UE_LOG(LogTemp,Display,TEXT("OK_CAMERA_PICKING: %s; 8 angles (%dx%d)"),CameraPassed ? TEXT("PASS") : TEXT("FAIL"),VW,VH);
        bPassed &= CameraPassed;
        ToggleRotationMode(); Orbit(13,500,-10000);
        bPassed &= FMath::IsNearlyEqual(CameraTilt,75.f) && FMath::IsNearlyEqual(CameraDistance,2900.f);
        Orbit(0,-500,10000);
        bPassed &= FMath::IsNearlyEqual(CameraTilt,35.f) && FMath::IsNearlyEqual(CameraDistance,6200.f);
        ToggleRotationMode(); ResetCamera();
        int32 Buildings=0,Cars=0;
        for (TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        {
            const FVector Size=It->GetStaticMeshComponent()->Bounds.BoxExtent*2;
            if (It->ActorHasTag(TEXT("Cabin4x4")))
            {
                ++Buildings; bPassed &= FMath::IsNearlyEqual(FMath::Max(Size.X,Size.Y),720.,1.);
            }
            if (It->ActorHasTag(TEXT("Vehicle3x2")))
            {
                ++Cars; bPassed &= Size.X<=541 && Size.Y<=361;
            }
        }
        bPassed &= Buildings==5 && Cars==1;
        UE_LOG(LogTemp,Display,TEXT("OK_COMPOSITION: %d buildings, %d car"),Buildings,Cars);
        for (auto A : FindSolution(0)) Act(A);
        UE_LOG(LogTemp,Display,TEXT("OK_DEMO_SMOKE: %s"),bPassed ? TEXT("PASS") : TEXT("FAIL"));
        FTimerHandle H;
        GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateLambda([bPassed]()
        { FPlatformMisc::RequestExitWithStatus(false,bPassed ? 0 : 1); }),16.f,false);
    });
    Later(16.f,[] { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DemoWin.png"),true,false); });
    Later(18.f,[this] { SelectLevel(1); });
    Later(20.f,[] { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DemoLevel2.png"),true,false); });
    Later(22.f,[this] { SelectLevel(2); });
    Later(24.f,[] { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DemoLevel3.png"),true,false); });
    Later(26.f,[this] { SetMenuPage(2); });
    Later(28.f,[] { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DemoOptions.png"),true,false); });
#endif
}
