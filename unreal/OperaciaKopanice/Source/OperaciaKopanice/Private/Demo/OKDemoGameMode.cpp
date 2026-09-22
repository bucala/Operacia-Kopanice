#include "Demo/OKDemoGameMode.h"
#include "Demo/OKDemoPlayerController.h"
#include "Demo/OKDemoHUD.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "Navigation/OKViaductActor.h"
#include "Architecture/OKModularBuildingTemplateActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"

AOKDemoGameMode::AOKDemoGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = AOKDemoPlayerController::StaticClass();
    HUDClass = AOKDemoHUD::StaticClass();
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    Cube = CubeAsset.Object; Sphere = SphereAsset.Object; Cylinder = CylinderAsset.Object;
    ShapeMaterial = MaterialAsset.Object;
}
FVector AOKDemoGameMode::CellLocation(FIntPoint Cell, float Z) const
{
    return FVector(Cell.X * OKDemo::CellSize, Cell.Y * OKDemo::CellSize, Z);
}
AStaticMeshActor* AOKDemoGameMode::Mesh(FVector Location, FVector Scale, FLinearColor Color, UStaticMesh* Asset)
{
    auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    auto* Component = Actor->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Asset ? Asset : Cube.Get());
    Component->SetMaterial(0, ShapeMaterial);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Actor->SetActorScale3D(Scale);
    Tint(Actor, Color);
    return Actor;
}
void AOKDemoGameMode::Tint(AStaticMeshActor* Actor, FLinearColor Color)
{
    auto* Component = Actor->GetStaticMeshComponent();
    auto* Material = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
    if (!Material) Material = Component->CreateDynamicMaterialInstance(0);
    if (Material) Material->SetVectorParameterValue(TEXT("Color"), Color);
}
void AOKDemoGameMode::StartPlay()
{
    Super::StartPlay();
    BuildScene();
    GConfig->GetBool(TEXT("Kopanice.Options"),TEXT("Grid"),bGridVisible,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Kopanice.Options"),TEXT("Camera"),bCameraEnabled,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Kopanice.Options"),TEXT("Stepped"),bSteppedRotation,GGameUserSettingsIni);
    GConfig->GetInt(TEXT("Kopanice.Options"),TEXT("Quality"),Quality,GGameUserSettingsIni);
    Quality=FMath::Clamp(Quality,0,3);
    Restart();
    MenuPage=FParse::Param(FCommandLine::Get(),TEXT("OKDemoSmoke")) ? 0 : 1;
    SetupSmokeTest();
}
void AOKDemoGameMode::BuildScene()
{
    auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1500), FRotator(-55,-40,0));
    Sun->GetLightComponent()->SetIntensity(4.8f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f,.96f,.89f));
    CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent())->LightSourceAngle=2.0f;
    CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1);
    auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1500), FRotator(-45,140,0));
    Fill->GetLightComponent()->SetIntensity(1.6f);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.67f,.79f,1.0f));
    Fill->GetLightComponent()->SetCastShadows(false);
    auto* Sky = GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(1.0f);
    Sky->GetLightComponent()->SetLowerHemisphereColor(FLinearColor(.25,.28,.3));
    BuildEnvironment();
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    for (int32 Y=0; Y<OKDemo::Height; ++Y)
        for (int32 X=0; X<OKDemo::Width; ++X)
        {
            const FIntPoint Cell(X,Y);
            auto* Tile=Mesh(CellLocation(Cell,bContinuousTerrain ? 3.f : -18.f),
                bContinuousTerrain ? FVector(1.8,1.8,1) : FVector(1.72,1.72,.24),
                FLinearColor::White,bContinuousTerrain ? Plane : nullptr);
            if (bContinuousTerrain)
            {
                auto* Component=Tile->GetStaticMeshComponent();
                Component->SetMaterial(0,OverlayMaterial);
                Component->CreateDynamicMaterialInstance(0);
                Component->SetCastShadow(false);
            }
            Tiles.Add(Tile);
            const bool bCabinCell = OKDemo::IsCabinCell(Cell);
            if (OKDemo::IsBlocked(Cell,false) && X != 6 && !bCabinCell)
            {
                if (Y != 2 && Y != 3)
                {
                    BuildWinterTree(CellLocation(Cell,0), X+Y*OKDemo::Width);
                }
                else
                {
                    if (auto* RockAsset=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Forest/SM_OK_ForestRock.SM_OK_ForestRock")))
                    {
                        auto* Rock=GetWorld()->SpawnActor<AStaticMeshActor>(CellLocation(Cell,-3),FRotator(0,24,0));
                        auto* Component=Rock->GetStaticMeshComponent();
                        Component->SetMobility(EComponentMobility::Movable);
                        Component->SetStaticMesh(RockAsset);
                        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                        Component->SetCanEverAffectNavigation(false);
                        Rock->SetActorScale3D(FVector(.55));
                    }
                    else
                    {
                    auto* Rock = Mesh(CellLocation(Cell,25), FVector(.65,.55,.55), FLinearColor(.34f,.37f,.38f));
                    Rock->SetActorRotation(FRotator(12,24,15));
                    Mesh(CellLocation(Cell,51), FVector(.65,.55,.13), FLinearColor(.83f,.90f,.95f), Sphere);
                    }
                }
            }
        }
    auto* Water=Mesh(CellLocation({6,4},-39), FVector(3.5,120,1), FLinearColor(.12f,.32f,.4f),Plane);
    Water->GetStaticMeshComponent()->SetCastShadow(false);
    if (auto* WaterMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_Water.M_Water")))
        Water->GetStaticMeshComponent()->SetMaterial(0,WaterMaterial);
    Bridge = GetWorld()->SpawnActor<AOKViaductActor>(CellLocation({6,4},0), FRotator::ZeroRotator);
    Bridge->BuildDemoModel();
    auto* DetailedCabin=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Cabin/SM_OK_Cabin.SM_OK_Cabin"));
    if (!DetailedCabin)
        DetailedCabin=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/DetailedCabin/SM_OK_CabinDetailed.SM_OK_CabinDetailed"));
    if (DetailedCabin)
    {
        auto* Cabin=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(450,270,-65),FRotator::ZeroRotator);
        auto* Component=Cabin->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(DetailedCabin);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FVector Size=DetailedCabin->GetBoundingBox().GetSize();
        Cabin->SetActorScale3D(FVector(OKDemo::CabinCells*OKDemo::CellSize/FMath::Max(1.0,FMath::Max(Size.X,Size.Y))));
        Cabin->Tags.Add(TEXT("Cabin4x4"));
        UE_LOG(LogTemp,Display,TEXT("OK_DETAIL_CABIN: imported mesh loaded"));
    }
    else
    {
        auto* Cabin=GetWorld()->SpawnActor<AOKModularBuildingTemplateActor>(FVector(450,270,0),FRotator::ZeroRotator);
        Cabin->BayCountX=1; Cabin->BayCountY=1; Cabin->BaySize=720; Cabin->WallHeight=440;
        Cabin->bGeneratePorch=false;
        Cabin->RebuildTemplate();
    }
    // Park outside the tactical cells: scenery must not obscure a legal route.
    if (auto* CarMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Car/SM_OK_Car.SM_OK_Car")))
    {
        auto* Car=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(1620,1710,0),FRotator::ZeroRotator);
        auto* Component=Car->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(CarMesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        const FVector Size=CarMesh->GetBoundingBox().GetSize();
        Car->SetActorScale3D(FVector(FMath::Min(540.0/Size.X,360.0/Size.Y)));
        Car->Tags.Add(TEXT("Vehicle3x2"));
        UE_LOG(LogTemp,Display,TEXT("OK_SUPPLIED_CAR: imported mesh loaded"));
    }
    PlayerMarker = BuildUnit(false);
    TNTMarker = Mesh(CellLocation({3,7},30), FVector(.65,.55,.5), FLinearColor(.95,.6,.08));
    DetonatorMarker=Mesh(CellLocation({8,4},35), FVector(.4,.4,.7), FLinearColor(.95,.6,.08));
    ExitMarker=Mesh(CellLocation({9,1},35), FVector(.12,.12,.7), FLinearColor(.1,.8,.75));
    for (int32 I=0; I<2; ++I)
    {
        Guards.Add(BuildUnit(true));
    }
    const FVector Center(900,720,0);
    Camera = GetWorld()->SpawnActor<ACameraActor>(Center+FVector(1420,2350,3500), FRotator::ZeroRotator);
    Camera->SetActorRotation((Center-Camera->GetActorLocation()).Rotation());
    Camera->GetCameraComponent()->SetFieldOfView(60);
    Camera->GetCameraComponent()->bConstrainAspectRatio = false;
    // Snow dominates the histogram; compensate so it reads as snow, not middle gray.
    auto& PostProcess=Camera->GetCameraComponent()->PostProcessSettings;
    PostProcess.bOverride_AutoExposureBias=true;
    PostProcess.AutoExposureBias=1.8f;
    PostProcess.bOverride_BloomIntensity=true;
    PostProcess.BloomIntensity=.1f;
    if (FParse::Param(FCommandLine::Get(),TEXT("OKCabinReview")))
    {
        const FVector Target(450,450,135);
        Camera->SetActorLocation(Target+FVector(600,700,530));
        Camera->SetActorRotation((Target-Camera->GetActorLocation()).Rotation());
    }
    if (auto* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
}
void AOKDemoGameMode::RefreshScene()
{
    const auto& Level=OKDemo::GetLevel(State.LevelIndex);
    Bridge->SetActorLocation(CellLocation({6,Level.BridgeRow}));
    TNTMarker->SetActorLocation(CellLocation(Level.TNT,30));
    DetonatorMarker->SetActorLocation(CellLocation(Level.Detonator,35));
    ExitMarker->SetActorLocation(CellLocation(Level.Exit,35));
    PlayerMarker->SetActorLocation(CellLocation(State.Player,0));
    FIntPoint PlayerFacing(0,-1);
    // Recover the last movement direction from snapshots, including after undo/wait.
    for (int32 I=History.Num()-1; I>=0; --I)
    {
        const FIntPoint Delta=State.Player-History[I].Player;
        if (Delta != FIntPoint::ZeroValue) { PlayerFacing=Delta; break; }
    }
    PlayerMarker->SetActorRotation(FVector(PlayerFacing.X,PlayerFacing.Y,0).Rotation());
    TNTMarker->SetActorHiddenInGame(State.bTNT);
    if (State.bBridgeDestroyed) Bridge->DestroySpan(); else Bridge->RepairSpan();
    for (int32 I=0; I<2; ++I)
    {
        const FIntPoint Position=OKDemo::GuardPosition(I,State.Turn,State.LevelIndex);
        const FIntPoint Facing=OKDemo::GuardFacing(I,State.Turn,State.LevelIndex);
        Guards[I]->SetActorLocation(CellLocation(Position,0));
        Guards[I]->SetActorRotation(FVector(Facing.X,Facing.Y,0).Rotation());
    }
    for (int32 Y=0; Y<OKDemo::Height; ++Y)
        for (int32 X=0; X<OKDemo::Width; ++X)
        {
            const FIntPoint Cell(X,Y);
            if (bContinuousTerrain)
            {
                auto* Tile=Tiles[Y*OKDemo::Width+X].Get();
                const bool bBlocked=OKDemo::IsBlocked(Cell,State.bBridgeDestroyed,State.LevelIndex);
                Tile->SetActorHiddenInGame(bBlocked);
                FLinearColor Overlay(.80f,.87f,.93f);
                float FillAmount=0;
                if ((Cell==Level.TNT && !State.bTNT) ||
                    (Cell==Level.Detonator && !State.bBridgeDestroyed))
                { Overlay=FLinearColor(.85f,.64f,.22f); FillAmount=.15f; }
                if (Cell==Level.Exit)
                { Overlay=FLinearColor(.3f,.7f,.6f); FillAmount=.17f; }
                if (OKDemo::IsThreatened(Cell,State))
                { Overlay=FLinearColor(.82f,.19f,.15f); FillAmount=.25f; }
                if (Cell==State.Player)
                { Overlay=FLinearColor(.22f,.7f,.47f); FillAmount=.17f; }
                Tint(Tile,Overlay);
                if (auto* Material=Cast<UMaterialInstanceDynamic>(Tile->GetStaticMeshComponent()->GetMaterial(0)))
                {
                    Material->SetScalarParameterValue(TEXT("Fill"),FillAmount);
                    Material->SetScalarParameterValue(TEXT("GridVisibility"),bGridVisible ? 1.f : 0.f);
                }
                continue;
            }
            FLinearColor Color(.70f,.78f,.82f);
            if (X==6) Color=FLinearColor(.12,.3,.36);
            if (OKDemo::IsBlocked(Cell,State.bBridgeDestroyed,State.LevelIndex) && X!=6) Color=FLinearColor(.56f,.64f,.68f);
            if (Cell==Level.TNT && !State.bTNT) Color=FLinearColor(.65,.44,.08);
            if (Cell==Level.Detonator && !State.bBridgeDestroyed) Color=FLinearColor(.65,.44,.08);
            if (Cell==Level.Exit) Color=FLinearColor(.08,.53,.46);
            if (OKDemo::IsThreatened(Cell,State)) Color=FLinearColor(.58,.14,.1);
            if (Cell==State.Player) Color=FLinearColor(.12,.65,.42);
            Tint(Tiles[Y*OKDemo::Width+X],Color);
        }
}
void AOKDemoGameMode::Act(OKDemo::EAction Action)
{
    if (MenuPage!=0) return;
    OKDemo::FState Next=State;
    if (!OKDemo::Apply(Next,Action))
    {
        Message=TEXT("Akcia nie je dostupna.");
        return;
    }
    auto* Turns=GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>();
    if (!Turns->CommitPlayerAction()) return;
    History.Add(State);
    State=Next;
    Turns->CompleteEnemyReaction();
    Turns->CompleteOutcomeEvaluation();
    Message=State.Outcome==OKDemo::EOutcome::Caught ? TEXT("Odhaleny. Vrat tah alebo restartuj misiu.") :
        State.Outcome==OKDemo::EOutcome::Won ? TEXT("Viadukt zniceny. Jednotka unikla.") : TEXT("");
    RefreshScene();
}
void AOKDemoGameMode::SyncTurns()
{
    auto* Turns=GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>();
    Turns->ResetTurns();
    // Restoring a snapshot must not replay enemy phase delegates.
    Turns->TurnNumber=State.Turn;
}
void AOKDemoGameMode::Restart()
{
    State=OKDemo::InitialState(State.LevelIndex); History.Reset(); SyncTurns();
    Message=OKDemo::GetLevel(State.LevelIndex).Name;
    RefreshScene();
}
void AOKDemoGameMode::Undo()
{
    if (MenuPage!=0) return;
    if (History.IsEmpty()) return;
    State=History.Pop(); SyncTurns(); Message=TEXT(""); RefreshScene();
}
void AOKDemoGameMode::ClickCell(FIntPoint Cell)
{
    const FIntPoint Delta=Cell-State.Player;
    if (Delta==FIntPoint(0,0))
    {
        const auto& Level=OKDemo::GetLevel(State.LevelIndex);
        if ((Cell==Level.TNT && !State.bTNT) || (Cell==Level.Detonator && State.bTNT && !State.bBridgeDestroyed))
            Act(OKDemo::EAction::Interact);
        else Act(OKDemo::EAction::Wait);
    }
    else if (Delta==FIntPoint(0,-1)) Act(OKDemo::EAction::North);
    else if (Delta==FIntPoint(1,0)) Act(OKDemo::EAction::East);
    else if (Delta==FIntPoint(0,1)) Act(OKDemo::EAction::South);
    else if (Delta==FIntPoint(-1,0)) Act(OKDemo::EAction::West);
}
FString AOKDemoGameMode::Objective() const
{
    if (State.Outcome==OKDemo::EOutcome::Won) return TEXT("MISIA SPLNENA");
    if (State.Outcome==OKDemo::EOutcome::Caught) return TEXT("ODHALENIE");
    if (!State.bTNT) return TEXT("1 / 3   Vyzdvihni TNT v sklade");
    if (!State.bBridgeDestroyed) return TEXT("2 / 3   Prejdi most a aktivuj rozbusku");
    return TEXT("3 / 3   Ustup k vychodnemu unikovemu bodu");
}
