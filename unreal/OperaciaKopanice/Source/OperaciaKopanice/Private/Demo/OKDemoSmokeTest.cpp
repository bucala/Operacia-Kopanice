#include "Demo/OKDemoGameMode.h"
#include "Demo/OKDemoHUD.h"
#include "Demo/OKDemoHUDLayout.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
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
                    Valid &= S.X>8 && S.X<VW-8 && S.Y>8 && S.Y<VH-8 && !FMath::IsNearlyZero(Direction.Z);
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
        bool HUDPassed=true;
        for (const FIntPoint View : {FIntPoint(1920,1080),FIntPoint(1280,720),FIntPoint(960,540),FIntPoint(640,480),FIntPoint(480,800)})
        {
            const auto L=OKHUD::Layout(View.X,View.Y);
            const TArray<FBox2D> Regions={L.Officer,L.Partisan,L.Objective,L.Message,
                FBox2D(L.Camera,L.Camera+FVector2D(200,96)),FBox2D(L.Movement,L.Movement+FVector2D(148,148))};
            for (int32 I=0;I<Regions.Num();++I)
            {
                const auto& B=Regions[I];
                HUDPassed &= B.Min.X>=0 && B.Min.Y>=0 && B.Max.X<=View.X && B.Max.Y<=View.Y;
                for (int32 J=I+1;J<Regions.Num();++J)
                {
                    const auto& Other=Regions[J];
                    HUDPassed &= !(B.Min.X<Other.Max.X && B.Max.X>Other.Min.X && B.Min.Y<Other.Max.Y && B.Max.Y>Other.Min.Y);
                }
            }
        }
        if (auto* HUD=Cast<AOKDemoHUD>(PC->GetHUD()))
        {
            const auto L=OKHUD::Layout(VW,VH);
            HUDPassed &= !HUD->HandleClick(300,8) && !HUD->HandleClick(VW*.5f,VH-8.f);
            HUDPassed &= HUD->HandleClick(L.Objective.Min.X+4,L.Objective.Min.Y+4);
            FState Expected=State; Apply(Expected,EAction::North);
            HUDPassed &= HUD->HandleClick(L.Movement.X+74,L.Movement.Y+22);
            HUDPassed &= State.Player==Expected.Player && State.Turn==Expected.Turn && State.Outcome==Expected.Outcome;
            if (State.Turn!=Before) HUDPassed &= HUD->HandleClick(L.Movement.X+126,L.Movement.Y+126);
            HUDPassed &= State.Turn==Before;
            HUDPassed &= HUD->HandleClick(L.Camera.X+178,L.Camera.Y+74) && !IsGridVisible();
            HUDPassed &= CheckScene();
            HUDPassed &= HUD->HandleClick(L.Camera.X+178,L.Camera.Y+74) && IsGridVisible();
            HUDPassed &= HUD->HandleClick(L.Objective.Max.X-30,L.Objective.Min.Y+30) && MenuPage==1;
            HUDPassed &= HUD->HandleClick(300,8);
            SetMenuPage(0);
        }
        else HUDPassed=false;
        UE_LOG(LogTemp,Display,TEXT("OK_REFERENCE_HUD: %s; five layouts, input, undo, menu and grid"),HUDPassed ? TEXT("PASS") : TEXT("FAIL"));
        bPassed &= HUDPassed;
        bool PanPassed=true;
        ResetCamera();
        const FVector Center=CameraTarget;
        PanCamera(1,0);
        PanPassed &= !CameraTarget.Equals(Center,.01);
        PanCamera(-1,0);
        PanPassed &= CameraTarget.Equals(Center,.01);
        const int32 CameraTurn=State.Turn;
        const int32 CameraHistory=History.Num();
        SetMenuPage(1); PanCamera(1,1); FocusPlayer();
        PanPassed &= CameraTarget.Equals(Center,.01);
        SetMenuPage(0); ToggleCamera(); PanCamera(1,1); FocusPlayer();
        PanPassed &= CameraTarget.Equals(Center,.01);
        ToggleCamera();
        UE_LOG(LogTemp,Display,TEXT("OK_CAMERA_PAN_GATES: %s"),PanPassed ? TEXT("PASS") : TEXT("FAIL"));
        for (int32 Step=0;Step<8;++Step)
        {
            PanCamera(10000,10000);
            PanPassed &= CameraTarget.X>=0 && CameraTarget.X<=(Width-1)*CellSize &&
                CameraTarget.Y>=0 && CameraTarget.Y<=(Height-1)*CellSize && CameraTarget.Z==0;
            FocusPlayer();
            PC->PlayerCameraManager->UpdateCamera(0);
            FVector2D Screen; FVector RayOrigin,RayDirection;
            PanPassed &= PC->ProjectWorldLocationToScreen(CellLocation(State.Player),Screen);
            PanPassed &= Screen.Equals(FVector2D(VW*.5,VH*.5),1.0);
            PanPassed &= PC->DeprojectScreenPositionToWorld(Screen.X,Screen.Y,RayOrigin,RayDirection);
            UE_LOG(LogTemp,Display,TEXT("OK_CAMERA_FOCUS_%d: target=%s screen=%s center=%dx%d"),Step,*CameraTarget.ToString(),*Screen.ToString(),VW/2,VH/2);
            if (FMath::IsNearlyZero(RayDirection.Z)) PanPassed=false;
            else
            {
                const FVector Hit=RayOrigin-RayDirection*(RayOrigin.Z/RayDirection.Z);
                UE_LOG(LogTemp,Display,TEXT("OK_CAMERA_RAY_%d: error=%.6f cm"),Step,FVector::Distance(Hit,CellLocation(State.Player)));
                PanPassed &= FIntPoint(FMath::RoundToInt(Hit.X/CellSize),FMath::RoundToInt(Hit.Y/CellSize))==State.Player;
            }
            Orbit(45,0);
            PanPassed &= CameraTarget.Equals(CellLocation(State.Player),.01);
        }
        PanPassed &= State.Turn==CameraTurn && History.Num()==CameraHistory;
        ResetCamera();
        PanPassed &= CameraTarget.Equals(Center,.01);
        UE_LOG(LogTemp,Display,TEXT("OK_CAMERA_PAN: %s; bounds, focus, picking and turn isolation"),PanPassed ? TEXT("PASS") : TEXT("FAIL"));
        bPassed &= PanPassed;
        ToggleRotationMode(); Orbit(13,500,-10000);
        bPassed &= FMath::IsNearlyEqual(CameraTilt,75.f) && FMath::IsNearlyEqual(CameraDistance,2900.f);
        Orbit(0,-500,10000);
        bPassed &= FMath::IsNearlyEqual(CameraTilt,35.f) && FMath::IsNearlyEqual(CameraDistance,6200.f);
        ToggleRotationMode(); ResetCamera();
        int32 Buildings=0,Cars=0,AlternateBuildings=0,CommandVans=0,WoodenPaths=0,Backdrops=0;
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
            if (It->ActorHasTag(TEXT("SuppliedCabinAlt"))) ++AlternateBuildings;
            if (It->ActorHasTag(TEXT("SuppliedCommandVan")))
            {
                ++CommandVans; bPassed &= Size.X<=901 && Size.Y<=361;
            }
            if (It->ActorHasTag(TEXT("ReferenceWoodenPaths"))) ++WoodenPaths;
            if (It->ActorHasTag(TEXT("WinterBackdrop")))
            {
                ++Backdrops;
                UE_LOG(LogTemp,Display,TEXT("OK_BACKDROP: location=%s collision=%d"),*It->GetActorLocation().ToString(),
                    int32(It->GetStaticMeshComponent()->GetCollisionEnabled()));
                bPassed &= It->GetActorLocation().Z<-100 &&
                    It->GetStaticMeshComponent()->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
            }
        }
        bPassed &= Buildings==5 && Cars==1 && AlternateBuildings==2 && CommandVans==1 && WoodenPaths==1 && Backdrops==1;
        UE_LOG(LogTemp,Display,TEXT("OK_COMPOSITION: %d buildings (%d alternate), %d car, %d van, %d paths, %d backdrop"),
            Buildings,AlternateBuildings,Cars,CommandVans,WoodenPaths,Backdrops);
        auto* CharacterAsset=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Partisan/SM_OK_Partisan.SM_OK_Partisan"));
        auto* OfficerAsset=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Officer/SM_OK_Officer.SM_OK_Officer"));
        bool CharactersPassed=CharacterAsset && OfficerAsset;
        for (AStaticMeshActor* Unit : {PlayerMarker.Get(),Guards[0].Get(),Guards[1].Get()})
        {
            auto* Expected=Unit==PlayerMarker.Get() ? CharacterAsset : OfficerAsset;
            TArray<UStaticMeshComponent*> Components;
            Unit->GetComponents(Components);
            int32 ImportedCount=0,VisibleMeshCount=0;
            for (auto* Component : Components)
            {
                if (Component->GetStaticMesh()) ++VisibleMeshCount;
                if (!Component->ComponentHasTag(TEXT("SuppliedCharacter"))) continue;
                ++ImportedCount;
                CharactersPassed &= Component->GetStaticMesh()==Expected;
                CharactersPassed &= Component->ComponentHasTag(Unit==PlayerMarker.Get() ? TEXT("PlayerCharacter") : TEXT("EnemyCharacter"));
                CharactersPassed &= Expected && Component->GetMaterial(0)==Expected->GetMaterial(0);
                CharactersPassed &= FMath::IsNearlyEqual(Component->Bounds.BoxExtent.Z*2,180.0,1.0);
            }
            // One source mesh plus one faction disc, with no primitive body parts.
            CharactersPassed &= ImportedCount==1 && VisibleMeshCount==2;
        }
        bPassed &= CharactersPassed;
        UE_LOG(LogTemp,Display,TEXT("OK_SUPPLIED_UNITS: %s; player and both guards"),CharactersPassed ? TEXT("PASS") : TEXT("FAIL"));
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
