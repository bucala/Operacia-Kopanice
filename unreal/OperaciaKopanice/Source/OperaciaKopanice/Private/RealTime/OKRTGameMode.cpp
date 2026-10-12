#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTSaveGame.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTNavBoundsVolume.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTHUD.h"
#include "Environment/OKWinterEnvironmentActor.h"
#include "Demo/OKObjectLayout.h"
#include "Navigation/OKViaductActor.h"
#include "Navigation/OKDynamicNavObstacleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"
#include "Scalability.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"

AOKRTGameMode::AOKRTGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    DefaultPawnClass=nullptr;
    PlayerControllerClass=AOKRTPlayerController::StaticClass();
    HUDClass=AOKRTHUD::StaticClass();
    PreviewSpline=CreateDefaultSubobject<USplineComponent>(TEXT("NavigationPreview"));
    SetRootComponent(PreviewSpline);
    PreviewSpline->SetCollisionProfileName(TEXT("NoCollision"));
    PreviewSpline->SetCanEverAffectNavigation(false);
}
AStaticMeshActor* AOKRTGameMode::Place(const TCHAR* Path,FVector Location,float Scale)
{
    auto* Asset=LoadObject<UStaticMesh>(nullptr,Path);
    if (!Asset) { UE_LOG(LogTemp,Error,TEXT("OK_RT_ASSET: missing %s"),Path); return nullptr; }
    auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(Location,FRotator::ZeroRotator);
    auto* Mesh=Actor->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(Asset);
    Mesh->SetCollisionProfileName(TEXT("NoCollision"));
    Mesh->SetCanEverAffectNavigation(false);
    Actor->SetActorScale3D(FVector(Scale));
    return Actor;
}
UBoxComponent* AOKRTGameMode::Collider(FVector Center,FVector Extent,FName Tag)
{
    auto* Actor=GetWorld()->SpawnActor<AActor>(Center,FRotator::ZeroRotator);
    auto* Box=NewObject<UBoxComponent>(Actor);
    Actor->SetRootComponent(Box);
    Actor->AddInstanceComponent(Box);
    Box->SetBoxExtent(Extent);
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->SetCanEverAffectNavigation(true);
    Box->RegisterComponent();
    Box->SetWorldLocation(Center);
    Actor->Tags.Add(Tag);
    return Box;
}
void AOKRTGameMode::StartPlay()
{
    Super::StartPlay();
    bMissionSmoke=FParse::Param(FCommandLine::Get(),TEXT("OKRTMissionSmoke"));
    bCampaignSmoke=FParse::Param(FCommandLine::Get(),TEXT("OKRTCampaignSmoke"));
    bRealtimeSmoke=FParse::Param(FCommandLine::Get(),TEXT("OKRTRealtimeSmoke"));
    bModernSmoke=FParse::Param(FCommandLine::Get(),TEXT("OKRTModernSmoke"));
    bSmoke=bModernSmoke || bCampaignSmoke || bMissionSmoke || bRealtimeSmoke || FParse::Param(FCommandLine::Get(),TEXT("OKRTSmoke"));
    const FString Requested=UGameplayStatics::ParseOption(OptionsString,TEXT("Mission"));
    if (Requested.IsEmpty()) FParse::Value(FCommandLine::Get(),TEXT("OKMission="),MissionId);
    else MissionId=FCString::Atoi(*Requested);
    MissionId=FMath::Clamp(MissionId,0,OKMissions::Count-1); PreviewMission=MissionId;
    const auto& Definition=Mission();
    TNTLocation=Definition.Supply; DetonatorLocation=Definition.Target;
    ExitLocation=Definition.Exit; HideLocation=Definition.Hide;
    bFrontEnd=!bSmoke && !UGameplayStatics::HasOption(OptionsString,TEXT("Deploy")) &&
        !FParse::Param(FCommandLine::Get(),TEXT("OKQuickStart"));
    SmokeStarted=StageStarted=FPlatformTime::Seconds();
    if (!bSmoke) LoadPreferences();
    BuildScene();
    DressScene();
#if WITH_EDITOR
    // Imported editor assets may still have pending render-data compilation.
    // Complete it before entering a paused menu or command-planning view.
    FAssetCompilingManager::Get().FinishAllCompilation();
#endif
    if (UGameplayStatics::HasOption(OptionsString,TEXT("QuickLoad")))
    {
        PendingLoad=Cast<UOKRTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot(),0));
        if (ValidateSnapshot(PendingLoad) && PendingLoad->bBridge) BreakBridge();
        if (bModernSmoke) SmokeStage=100;
    }
    if (bCampaignSmoke)
    {
        TogglePause(); ToggleMenu();
        Require(!bTacticalPause && !bMenu && !UGameplayStatics::IsGamePaused(GetWorld()),
            TEXT("startup pause and menu cannot freeze navigation construction"));
    }
    for (AOKRTUnit* Unit:Enemies) Unit->Vision->bConeVisible=bCones;
    Message=TEXT("Infiltracia zacala.");
}
void AOKRTGameMode::LoadPreferences()
{
    const TCHAR* Section=TEXT("OperaciaKopanice.RealTime");
    GConfig->GetBool(Section,TEXT("Cones"),bCones,GGameUserSettingsIni);
    GConfig->GetBool(Section,TEXT("PathPreview"),bPathPreview,GGameUserSettingsIni);
    GConfig->GetBool(Section,TEXT("SteppedCamera"),bSteppedCamera,GGameUserSettingsIni);
    GConfig->GetInt(Section,TEXT("Quality"),Quality,GGameUserSettingsIni);
    GConfig->GetBool(Section,TEXT("ObjectiveMarkers"),bObjectiveMarkers,GGameUserSettingsIni);
    GConfig->GetBool(Section,TEXT("PartyMarkers"),bPartyMarkers,GGameUserSettingsIni);
    GConfig->GetFloat(Section,TEXT("CameraSensitivity"),CameraSensitivity,GGameUserSettingsIni);
    GConfig->GetInt(Section,TEXT("CompletedMissions"),CompletedMissions,GGameUserSettingsIni);
    CameraSensitivity=FMath::Clamp(CameraSensitivity,.5f,2.f);
    Quality=FMath::Clamp(Quality,0,3);
    Scalability::FQualityLevels Levels;
    Levels.SetFromSingleQualityLevel(Quality);
    Scalability::SetQualityLevels(Levels);
}
void AOKRTGameMode::SavePreferences()
{
    // Integration fixtures must not overwrite the player's preferences.
    if (bSmoke) return;
    const TCHAR* Section=TEXT("OperaciaKopanice.RealTime");
    GConfig->SetBool(Section,TEXT("Cones"),bCones,GGameUserSettingsIni);
    GConfig->SetBool(Section,TEXT("PathPreview"),bPathPreview,GGameUserSettingsIni);
    GConfig->SetBool(Section,TEXT("SteppedCamera"),bSteppedCamera,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Quality"),Quality,GGameUserSettingsIni);
    GConfig->SetBool(Section,TEXT("ObjectiveMarkers"),bObjectiveMarkers,GGameUserSettingsIni);
    GConfig->SetBool(Section,TEXT("PartyMarkers"),bPartyMarkers,GGameUserSettingsIni);
    GConfig->SetFloat(Section,TEXT("CameraSensitivity"),CameraSensitivity,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("CompletedMissions"),CompletedMissions,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
    if (!bPathPreview) PreviewSpline->ClearSplinePoints();
}
void AOKRTGameMode::ToggleCones()
{
    bCones=!bCones;
    for (AOKRTUnit* Unit:Enemies) Unit->Vision->bConeVisible=bCones;
    SavePreferences();
}
void AOKRTGameMode::CycleQuality()
{
    Quality=(Quality+1)%4;
    Scalability::FQualityLevels Levels;
    Levels.SetFromSingleQualityLevel(Quality);
    Scalability::SetQualityLevels(Levels);
    SavePreferences();
}
void AOKRTGameMode::BuildScene()
{
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1500),FRotator(-55,-40,0));
    Sun->GetLightComponent()->SetIntensity(5.2f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.96f,.89f));
    auto* SunComponent=CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
    SunComponent->SetForwardShadingPriority(1);
    SunComponent->LightSourceAngle=5; SunComponent->SetAtmosphereSunLight(true);
    auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1500),FRotator(-45,140,0));
    Fill->GetLightComponent()->SetIntensity(.65f);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.67f,.79f,1));
    Fill->GetLightComponent()->SetCastShadows(false);
    CastChecked<UDirectionalLightComponent>(Fill->GetLightComponent())->SetAtmosphereSunLight(false);
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(.85f);
    Sky->GetLightComponent()->SetLightColor(FLinearColor(.72f,.82f,1.f));
    if (auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(900,720,80),FRotator::ZeroRotator))
    {
        auto* FogComponent=Fog->GetComponent();
        FogComponent->SetFogDensity(.012f);
        FogComponent->SetFogHeightFalloff(.18f);
        FogComponent->SetFogInscatteringColor(FLinearColor(.72f,.82f,.94f));
        FogComponent->SetDirectionalInscatteringExponent(4.f);
        FogComponent->SetDirectionalInscatteringStartDistance(700.f);
        FogComponent->SetDirectionalInscatteringColor(FLinearColor(.88f,.93f,1.f));
        FogComponent->SetSecondFogDensity(.003f);
        FogComponent->SetSecondFogHeightFalloff(.08f);
        FogComponent->SetSecondFogHeightOffset(420.f);
        FogComponent->SetVolumetricFog(true);
        FogComponent->SetVolumetricFogExtinctionScale(.18f);
        FogComponent->SetVolumetricFogScatteringDistribution(.35f);
    }
    if (MissionId==0)
    {
    GetWorld()->SpawnActor<AOKWinterEnvironmentActor>()->BuildEnvironment();
    auto* Cabin=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Cabin/SM_OK_Cabin.SM_OK_Cabin"));
    if (Cabin)
    {
        const auto Size=Cabin->GetBoundingBox().GetSize();
        Place(TEXT("/Game/Kopanice/Supplied/Cabin/SM_OK_Cabin.SM_OK_Cabin"),FVector(450,270,-65),
            OKLayout::UniformScale(OKLayout::EObject::Cabin,Size.X,Size.Y));
    }
    auto* Car=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Kopanice/Supplied/Car/SM_OK_Car.SM_OK_Car"));
    if (Car)
    {
        const auto Size=Car->GetBoundingBox().GetSize();
        Place(TEXT("/Game/Kopanice/Supplied/Car/SM_OK_Car.SM_OK_Car"),FVector(1620,1710,0),
            OKLayout::UniformScale(OKLayout::EObject::Car,Size.X,Size.Y));
    }
    if (auto* Water=Place(TEXT("/Game/Kopanice/WinterGround/SM_OK_RiverSurface.SM_OK_RiverSurface"),FVector::ZeroVector))
    {
        Water->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_Water.M_Water")));
        Water->GetStaticMeshComponent()->SetCastShadow(false);
    }
    // Imported ground has no cooked collision. Separate bank surfaces leave a genuine unwalkable river.
    Collider(FVector(437.5,720,-18),FVector(527.5,810,10),TEXT("WalkableWest"));
    Collider(FVector(1542.5,720,-18),FVector(347.5,810,10),TEXT("WalkableEast"));
    Collider(FVector(450,270,235),FVector(340,340,250),TEXT("CabinCollision"));
    Collider(FVector(600,1440,55),FVector(50,65,63),TEXT("StealthCover"));
    Place(TEXT("/Game/Kopanice/WinterGround/SM_OK_SupplyCrate.SM_OK_SupplyCrate"),FVector(600,1440,16),1.7f);
    BridgeFloor=Collider(FVector(1080,720,0),FVector(140,110,11),TEXT("WalkableBridge"));
    Bridge=GetWorld()->SpawnActor<AOKViaductActor>(FVector(1080,720,0),FRotator::ZeroRotator);
    Bridge->BuildDemoModel();
    Bridge->IntactSpan->SetRelativeScale3D(FVector(2.6,1.5,.22));
    TArray<UStaticMeshComponent*> Parts; Bridge->GetComponents(Parts);
    for (auto* Part:Parts) { Part->SetCollisionProfileName(TEXT("NoCollision")); Part->SetCanEverAffectNavigation(false); }
    Bridge->GapBlocker->SetBoxExtent(FVector(140,110,150));
    TNTMarker=Place(TEXT("/Game/Kopanice/WinterGround/SM_OK_SupplyCrate.SM_OK_SupplyCrate"),TNTLocation+FVector(0,0,14));
    SpawnMissionUnit(Mission().Spawn,false,0,{});
    SpawnMissionUnit(Mission().Spawn+FVector(180,0,0),false,0,{});
    for (const auto& Guard:Mission().Guards)
        SpawnMissionUnit(Guard.Spawn,true,Guard.FacingYaw,Guard.Route,Guard.Duty);
    Select(0);
    }
    else BuildAdditionalMission();
    Camera=GetWorld()->SpawnActor<ACameraActor>();
    auto* Lens=Camera->GetCameraComponent();
    Lens->SetFieldOfView(60); Lens->bConstrainAspectRatio=false;
    Lens->bOverrideAspectRatioAxisConstraint=true;
    Lens->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
    Lens->PostProcessSettings.bOverride_AutoExposureBias=true;
    Lens->PostProcessSettings.AutoExposureBias=.25f;
    Lens->PostProcessSettings.bOverride_ColorSaturation=true;
    Lens->PostProcessSettings.ColorSaturation=FVector4(.93f,.96f,1.04f,1.f);
    Lens->PostProcessSettings.bOverride_ColorContrast=true;
    Lens->PostProcessSettings.ColorContrast=FVector4(1.04f,1.04f,1.04f,1.f);
    Lens->PostProcessSettings.bOverride_ColorGamma=true;
    Lens->PostProcessSettings.ColorGamma=FVector4(1.02f,1.02f,1.04f,1.f);
    Lens->PostProcessSettings.bOverride_BloomIntensity=true;
    Lens->PostProcessSettings.BloomIntensity=.1f;
    // Camera orbit and zoom are allowed while tactical pause is active. Do not
    // leave a stale camera-velocity blur in the paused render history.
    Lens->PostProcessSettings.bOverride_MotionBlurAmount=true;
    Lens->PostProcessSettings.MotionBlurAmount=0.f;
    Lens->PostProcessSettings.bOverride_MotionBlurMax=true;
    Lens->PostProcessSettings.MotionBlurMax=0.f;
    UpdateCamera();
    GetWorld()->GetFirstPlayerController()->SetViewTarget(Camera);
    const FVector NavCenter=MissionId==0 ? FVector(900,720,0) : FVector(Mission().MapSize/2,Mission().MapSize/2,0);
    auto* Bounds=GetWorld()->SpawnActor<AOKRTNavBoundsVolume>(NavCenter,FRotator::ZeroRotator);
    if (MissionId!=0) Bounds->Bounds->SetBoxExtent(FVector(Mission().MapSize/2+100,Mission().MapSize/2+100,500));
    if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    { Nav->OnNavigationBoundsUpdated(Bounds); Nav->Build(); }
}
void AOKRTGameMode::Select(int32 Index,bool Append)
{
    if (bMenu || bWon || bLost || !Party.IsValidIndex(Index) || !Party[Index]->IsAlive()) return;
    if (!Append) for (AOKRTUnit* Unit:Party) Unit->Select(false);
    Party[Index]->Select(true); ActiveMember=Index;
    PreviewSpline->ClearSplinePoints();
}
void AOKRTGameMode::SelectAll()
{
    if (bMenu || bWon || bLost) return;
    for (AOKRTUnit* Unit:Party) Unit->Select(Unit->IsAlive());
    if (!Party.IsValidIndex(ActiveMember) || !Party[ActiveMember]->IsAlive())
        for (int32 I=0;I<Party.Num();++I) if (Party[I]->IsAlive()) { ActiveMember=I; break; }
}
void AOKRTGameMode::ToggleSelection(int32 Index)
{
    if (bMenu || bWon || bLost || !Party.IsValidIndex(Index) || !Party[Index]->IsAlive()) return;
    if (!Party[Index]->IsSelected()) { Select(Index,true); return; }
    Party[Index]->Select(false);
    if (ActiveMember==Index)
        for (int32 I=0;I<Party.Num();++I)
            if (Party[I]->IsAlive() && Party[I]->IsSelected()) { ActiveMember=I; break; }
    PreviewSpline->ClearSplinePoints();
}
bool AOKRTGameMode::Command(FOKRTOrder Order,bool Append)
{
    if (bMenu || bWon || bLost) return false;
    int32 Accepted=0,Requested=0;
    FString Failure,AcceptedName;
    const bool Group=Order.Kind==EOKOrder::Move || Order.Kind==EOKOrder::Stance;
    for (AOKRTUnit* Unit:Party)
        if (Unit->IsSelected() && Unit->IsAlive() && (Group || (Party.IsValidIndex(ActiveMember) && Party[ActiveMember]==Unit)))
        {
            ++Requested;
            const auto Individual=IndividualOrder(Order,Unit);
            const bool Promoted=!bCommandMode && Order.Kind==EOKOrder::Move && Order.bRunToDestination &&
                (Append || bTacticalPause || Unit->QueueSize()==1) && Unit->PromoteLastMoveToRun(Individual.Location);
            if (Promoted || (bCommandMode ? StagePlan(Unit,Individual) : Unit->Submit(Individual,Append))) { ++Accepted; AcceptedName=Unit->DisplayName; }
            else if (Failure.IsEmpty()) Failure=Unit->DisplayName+TEXT(": ")+Message;
        }
    const FVector Target=Order.Target.IsValid() ? Order.Target->Feet() : Order.Location;
    const bool WorldTarget=Order.Kind==EOKOrder::Move || Order.Kind==EOKOrder::Distract || Order.Target.IsValid() ||
        (Order.Kind==EOKOrder::Interact && Order.bApproachInteraction);
    CommandFeedback={Target,Order.Kind,Accepted,Requested,FPlatformTime::Seconds(),WorldTarget};
    if (Requested==0) Message=TEXT("Vyber dostupnu postavu.");
    else if (Accepted==Requested) Message=Requested>1 ? FString::Printf(TEXT("Rozkaz prijaty: %d/%d."),Accepted,Requested) :
        AcceptedName+TEXT(": rozkaz prijaty.");
    else Message=Accepted>0 ? FString::Printf(TEXT("Prijate %d/%d. %s"),Accepted,Requested,*Failure) : Failure;
    return Accepted>0;
}
void AOKRTGameMode::TogglePause()
{
    if (bInitialPausePending || bWon || bLost || bMenu) return;
    if (bCommandMode) { ExecutePlan(); return; }
    bTacticalPause=!bTacticalPause;
    UGameplayStatics::SetGamePaused(GetWorld(),bTacticalPause);
    Message=bTacticalPause ? TEXT("Takticka pauza") : TEXT("Prikazy vykonavane.");
}
void AOKRTGameMode::ToggleMenu()
{
    if (bMenu) { MenuBack(); return; }
    OpenMenuPage(EOKMenuPage::Pause);
}
void AOKRTGameMode::OpenMenuPage(EOKMenuPage Page)
{
    if (bInitialPausePending) return;
    bMenu=true; MenuPage=Page; bOptions=Page==EOKMenuPage::Options;
    if (auto* PC=Cast<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController()))
    { PC->CancelPointer(); PC->bArmed=false; }
    PreviewSpline->ClearSplinePoints();
    UGameplayStatics::SetGamePaused(GetWorld(),true);
}
void AOKRTGameMode::MenuBack()
{
    if (MenuPage!=EOKMenuPage::Pause)
    { OpenMenuPage(MenuPage==EOKMenuPage::Briefing ? EOKMenuPage::Missions : EOKMenuPage::Pause); return; }
    if (bFrontEnd || bWon || bLost) return;
    bMenu=false; bOptions=false;
    UGameplayStatics::SetGamePaused(GetWorld(),bMenu || bTacticalPause);
}
void AOKRTGameMode::Restart()
{
    StartMission(MissionId);
}
void AOKRTGameMode::StartMission(int32 Id)
{
    if (Id<0 || Id>=OKMissions::Count) return;
    UGameplayStatics::SetGamePaused(GetWorld(),false);
    UGameplayStatics::OpenLevel(GetWorld(),TEXT("/Engine/Maps/Entry"),true,FString::Printf(TEXT("Mission=%d?Deploy=1"),Id));
}
void AOKRTGameMode::UpdateCamera()
{
    if (!Camera) return;
    const FVector Offset=FRotator(CameraTilt,CameraYaw,0).Vector()*CameraDistance;
    Camera->SetActorLocation(CameraTarget+Offset);
    Camera->SetActorRotation((-Offset).Rotation());
}
void AOKRTGameMode::Orbit(float Yaw,float Tilt,float Zoom)
{
    CameraYaw+=Yaw; CameraTilt=FMath::Clamp(CameraTilt+Tilt,25.f,75.f);
    CameraDistance=FMath::Clamp(CameraDistance+Zoom,1700.f,MissionId==0 ? 6500.f : 14500.f); UpdateCamera();
}
void AOKRTGameMode::Pan(float X,float Y)
{
    const FVector Right=Camera->GetActorRightVector().GetSafeNormal2D();
    const FVector Forward=Camera->GetActorForwardVector().GetSafeNormal2D();
    CameraTarget+=Right*X+Forward*Y;
    CameraTarget.X=FMath::Clamp(CameraTarget.X,-300.,MissionId==0 ? 2200. : double(Mission().MapSize+300));
    CameraTarget.Y=FMath::Clamp(CameraTarget.Y,-300.,MissionId==0 ? 2000. : double(Mission().MapSize+300)); UpdateCamera();
}
void AOKRTGameMode::FocusSelected()
{
    if (Party.IsValidIndex(ActiveMember)) CameraTarget=Party[ActiveMember]->Feet();
    UpdateCamera();
}
void AOKRTGameMode::FocusObjective()
{
    if (bMenu || bWon || bLost) return;
    const auto Stage=OKMissions::Stage(Mission(),WorldState());
    CameraTarget=Stage==EOKObjectiveStage::Extraction ? ExitLocation :
        Stage==EOKObjectiveStage::Supply || Mission().Goal==EOKMissionGoal::Documents ? TNTLocation : DetonatorLocation;
    UpdateCamera();
}
void AOKRTGameMode::Noise(AOKRTUnit* Source,FVector Location,float Loudness,float Range,FName Tag)
{
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),Location,Loudness,Source,Range,Tag);
    NoisePulses.Add({Location,Range,0});
}
void AOKRTGameMode::Alarm(AOKRTUnit* Target,AOKRTUnit* Source)
{
    Message=TEXT("Poplach!");
    for (AOKRTUnit* Guard:Enemies)
        if (Guard!=Source && FVector::Dist2D(Guard->Feet(),Source->Feet())<1400)
            CastChecked<AOKRTGuardController>(Guard->GetController())->Alert(Target,false);
}
bool AOKRTGameMode::Distract(AOKRTUnit* Unit,FVector Destination)
{
    if (FVector::Dist2D(Unit->Feet(),Destination)>800)
    { Message=TEXT("Ciel je prilis daleko."); return false; }
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRTThrow),false,Unit);
    if (GetWorld()->LineTraceSingleByChannel(Hit,Unit->Feet()+FVector(0,0,120),Destination+FVector(0,0,120),ECC_Visibility,Params))
    { Message=TEXT("Prekazka blokuje hod."); return false; }
    Noise(Unit,Destination,1,1500,TEXT("Distraction")); Message=TEXT("Odputanie pozornosti."); return true;
}
EOKInteraction AOKRTGameMode::FindInteractionAt(FVector PickedLocation,FVector& Destination) const
{
    if (!Party.IsValidIndex(ActiveMember) || !Party[ActiveMember]->IsAlive()) return EOKInteraction::Nearby;
    float Nearest=140.f;
    EOKInteraction Found=EOKInteraction::Nearby;
    auto Candidate=[&](FVector Location,EOKInteraction Kind)
    {
        const float Distance=FVector::Dist2D(PickedLocation,Location);
        if (Distance<Nearest) { Nearest=Distance; Destination=Location; Found=Kind; }
    };
    if (Mission().Goal==EOKMissionGoal::Documents)
    { if (!bObjectiveComplete) Candidate(TNTLocation,EOKInteraction::CollectDocuments); }
    else if (!bHasTNT) Candidate(TNTLocation,EOKInteraction::CollectTNT);
    // Resolve future objectives while planning; prerequisites are checked on arrival.
    if (Mission().Goal==EOKMissionGoal::Bridge && !bBridgeDestroyed) Candidate(DetonatorLocation,EOKInteraction::DetonateBridge);
    if (Mission().Goal==EOKMissionGoal::CommandPost && !bObjectiveComplete) Candidate(DetonatorLocation,EOKInteraction::SabotageCommandPost);
    Candidate(HideLocation,EOKInteraction::HideBody);
    return Found;
}
void AOKRTGameMode::Interact(AOKRTUnit* Unit,EOKInteraction Kind)
{
    if (!Unit || !Unit->IsAlive() || bWon || bLost) return;
    const auto State=WorldState();
    if (OKMissions::CanExecute(Mission(),State,EOKInteraction::CollectDocuments) &&
        (Kind==EOKInteraction::Nearby || Kind==EOKInteraction::CollectDocuments) && FVector::Dist2D(Unit->Feet(),TNTLocation)<140)
    { bObjectiveComplete=true; if (TNTMarker) TNTMarker->SetActorHiddenInGame(true); Message=TEXT("Dokumenty ziskane. Presunte cely tim k vychodu."); return; }
    if (OKMissions::CanExecute(Mission(),State,EOKInteraction::SabotageCommandPost) &&
        (Kind==EOKInteraction::Nearby || Kind==EOKInteraction::SabotageCommandPost) && FVector::Dist2D(Unit->Feet(),DetonatorLocation)<140)
    {
        bObjectiveComplete=true;
        Noise(Unit,DetonatorLocation,1,1600,TEXT("Explosion"));
        Message=TEXT("Velitelstvo vyradene. Ustupte spolu!"); return;
    }
    if ((Kind==EOKInteraction::Nearby || Kind==EOKInteraction::HideBody) &&
        Unit->CarriedBody.IsValid() && FVector::Dist2D(Unit->Feet(),HideLocation)<170)
    { Unit->DropBody(true); Message=TEXT("Telo ukryte."); return; }
    if ((Kind==EOKInteraction::Nearby || Kind==EOKInteraction::CollectTNT) &&
        OKMissions::CanExecute(Mission(),State,EOKInteraction::CollectTNT) && FVector::Dist2D(Unit->Feet(),TNTLocation)<140)
    { bHasTNT=true; if (TNTMarker) TNTMarker->SetActorHiddenInGame(true); Message=TEXT("TNT ziskane."); return; }
    if ((Kind==EOKInteraction::Nearby || Kind==EOKInteraction::DetonateBridge) &&
        Mission().Goal==EOKMissionGoal::Bridge && bHasTNT && !bBridgeDestroyed && FVector::Dist2D(Unit->Feet(),DetonatorLocation)<140)
    {
        if (!OKMissions::CanExecute(Mission(),State,EOKInteraction::DetonateBridge))
        { Message=TEXT("Najprv presunte cely tim na vychodny breh."); return; }
        BreakBridge();
        Noise(Unit,FVector(1080,720,0),1,1600,TEXT("Explosion"));
        Message=TEXT("Most zniceny. Ustupte spolu."); return;
    }
    Message=TEXT("Nie je tu dostupna interakcia.");
}
bool AOKRTGameMode::IsCover(FVector Location,float BodyHeight) const
{
    for (const auto& Shrub:FoliageCover)
        if (FVector::DistSquared2D(Location,Shrub.Base)<FMath::Square(Shrub.Radius) &&
            Location.Z>=Shrub.Base.Z-15 && Location.Z+BodyHeight<=Shrub.Base.Z+Shrub.Height) return true;
    for (const FVector Point:CoverLocations)
        if (FMath::Abs(Location.Z-Point.Z)<80 && FVector::Dist2D(Location,Point)<170) return true;
    return MissionId==0 && FMath::Abs(Location.Z)<80 && FVector::Dist2D(Location,FVector(600,1440,0))<170;
}
bool AOKRTGameMode::HasCompletePath(FVector From,FVector To) const
{
    auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),From,To);
    return Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num()>1;
}
void AOKRTGameMode::Preview(FVector Destination)
{
    FOKRTOrder Order; Order.Location=Destination; PreviewOrder(Order);
}
bool AOKRTGameMode::PreviewOrder(const FOKRTOrder& Order,bool Append)
{
    PreviewSpline->ClearSplinePoints(false);
    if (!bPathPreview || bMenu || bWon || bLost || !Party.IsValidIndex(ActiveMember)) return false;
    const auto* Unit=Party[ActiveMember].Get();
    if (!Unit->IsAlive() || !Unit->IsSelected()) return false;
    TArray<FVector> Points; FString Reason;
    if (!Unit->GetOrderPath(IndividualOrder(Order,Unit),!bCommandMode && (Append || bTacticalPause),Points,Reason) || Points.IsEmpty()) return false;
    for (const auto& Point:Points) PreviewSpline->AddSplinePoint(Point+FVector(0,0,8),ESplineCoordinateSpace::World,false);
    PreviewSpline->UpdateSpline();
    return true;
}
int32 AOKRTGameMode::PartyMembersOnSafeBank() const
{
    if (!BridgeFloor) return 0;
    // Mission 1 extracts east: the entire capsule must clear the disappearing span.
    const double SpanEnd=BridgeFloor->Bounds.GetBox().Max.X;
    int32 Count=0;
    for (const AOKRTUnit* Unit:Party)
        if (Unit && Unit->IsAlive() && Unit->Feet().X-Unit->GetCapsuleComponent()->GetScaledCapsuleRadius()>SpanEnd)
            ++Count;
    return Count;
}
FString AOKRTGameMode::Objective() const
{
    if (bLost) return TEXT("Misia zlyhala");
    if (bWon) return TEXT("Misia splnena");
    const auto State=WorldState();
    switch (OKMissions::Stage(Mission(),State))
    {
    case EOKObjectiveStage::Supply: return Mission().Goal==EOKMissionGoal::CommandPost ? TEXT("Ziskajte TNT pri sklade") : TEXT("Ziskajte TNT");
    case EOKObjectiveStage::Crossing: return FString::Printf(TEXT("Prejdite most: %d/%d v bezpeci"),State.SafeBankCount,State.PartyCount);
    case EOKObjectiveStage::Target: return Mission().Goal==EOKMissionGoal::Documents ? TEXT("Ziskajte dokumenty z tabora") :
        Mission().Goal==EOKMissionGoal::CommandPost ? TEXT("Vyradte velitelstvo") : TEXT("Aktivujte detonator");
    default: return TEXT("Dosiahnite vychod s celym timom");
    }
}
FOKRTMissionState AOKRTGameMode::WorldState() const
{
    FOKRTMissionState State;
    State.bSupplies=bHasTNT;
    State.bTargetComplete=Mission().Goal==EOKMissionGoal::Bridge ? bBridgeDestroyed : bObjectiveComplete;
    State.PartyCount=Party.Num(); State.SafeBankCount=PartyMembersOnSafeBank();
    for (const AOKRTUnit* Unit:Party)
        if (Unit && Unit->IsAlive())
        {
            ++State.AliveCount;
            if (FVector::Dist2D(Unit->Feet(),ExitLocation)<160) ++State.ExitedCount;
        }
    return State;
}
void AOKRTGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    const bool LoadedWreck=PendingLoad && ValidateSnapshot(PendingLoad) && PendingLoad->bBridge;
    const bool ReadyRoutes=bInitialPausePending && (LoadedWreck
        ? HasCompletePath(Party[0]->Feet(),TNTLocation) && HasCompletePath(PendingLoad->Units[0].Feet,ExitLocation)
        : HasCompletePath(Party[0]->Feet(),DetonatorLocation) && HasCompletePath(Party[0]->Feet(),ExitLocation));
    if (bInitialPausePending && !UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(GetWorld()) && ReadyRoutes)
    {
        bInitialPausePending=false;
        for (AOKRTUnit* Guard:Enemies)
        {
            CastChecked<AOKRTGuardController>(Guard->GetController())->bBrainEnabled=!bSmoke || bMissionSmoke || bCampaignSmoke || bRealtimeSmoke;
            Guard->Vision->Refresh();
        }
        // Production deployment runs immediately. Older planning fixtures opt in
        // to pause explicitly; the realtime fixture exercises the production path.
        if (bSmoke && !bRealtimeSmoke) TogglePause();
        if (bFrontEnd) OpenMenuPage(EOKMenuPage::Pause);
        if (PendingLoad)
        {
            if (!ApplySnapshot(PendingLoad)) Message=TEXT("Obnovenie ulozenej misie zlyhalo.");
            PendingLoad=nullptr;
        }
    }
    if (!bTacticalPause && !bMenu)
    {
        UpdateSynchronization();
        for (auto& Pulse:NoisePulses) Pulse.Age+=Delta;
        NoisePulses.RemoveAll([](const auto& P){ return P.Age>1.5f; });
        for (AOKRTUnit* Unit:Party)
        {
            if (Unit->Feet().Z<-220 && Unit->IsAlive()) Unit->TakeHit(100);
            if (!Unit->IsAlive()) bLost=true;
        }
        if (OKMissions::CanExtract(WorldState()) && !bLost && !bWon)
        { bWon=true; CompletedMissions|=1<<MissionId; SavePreferences(); }
        if ((bWon || bLost) && !UGameplayStatics::IsGamePaused(GetWorld()))
        { UGameplayStatics::SetGamePaused(GetWorld(),true); }
    }
    if (bModernSmoke) ModernSmokeTick();
    else if (bRealtimeSmoke) RealtimeSmokeTick();
    else if (bCampaignSmoke) CampaignSmokeTick();
    else if (bMissionSmoke) MissionSmokeTick(Delta);
    else if (bSmoke) SmokeTick();
}
