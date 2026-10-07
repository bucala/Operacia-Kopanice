#include "RealTime/OKRTGameMode.h"
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
    bSmoke=bCampaignSmoke || bMissionSmoke || FParse::Param(FCommandLine::Get(),TEXT("OKRTSmoke"));
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
    for (int32 I=0;I<4;++I)
    {
        const bool Enemy=I>=2;
        const FVector Location=I==0 ? FVector(300,1380,100) : I==1 ? FVector(480,1380,100) : I==2 ? FVector(660,700,100) : FVector(1740,1200,100);
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Unit=GetWorld()->SpawnActor<AOKRTUnit>(Location,FRotator(0,I==2 ? -90 : Enemy ? 90 : 0,0),Params);
        Unit->Initialize(Enemy,I!=0);
        if (Enemy)
        {
            Enemies.Add(Unit);
            auto* AI=CastChecked<AOKRTGuardController>(Unit->GetController());
            // Turn north before returning west, so the cone never sweeps the party spawn.
            AI->PatrolRoute=I==2 ? TArray<FOKRTPatrolStop>{
                {FVector(660,700,0),-90,12}, {FVector(840,700,0),0,2}, {FVector(840,700,0),-90,1}} :
                TArray<FOKRTPatrolStop>{
                {FVector(1740,1200,0),90,8}, {FVector(1740,1020,0),180,2}, {FVector(1740,1020,0),0,1}};
            AI->bBrainEnabled=false;
        }
        else Party.Add(Unit);
    }
    Select(0);
    }
    else BuildAdditionalMission();
    Camera=GetWorld()->SpawnActor<ACameraActor>();
    auto* Lens=Camera->GetCameraComponent();
    Lens->SetFieldOfView(60); Lens->bConstrainAspectRatio=false;
    Lens->bOverrideAspectRatioAxisConstraint=true;
    Lens->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
    Lens->PostProcessSettings.bOverride_AutoExposureBias=true;
    Lens->PostProcessSettings.AutoExposureBias=1.15f;
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
bool AOKRTGameMode::Command(FOKRTOrder Order,bool Append)
{
    if (bMenu || bWon || bLost) return false;
    // Movement and stance affect the selection; scarce skills belong to the active portrait.
    if (Order.Kind!=EOKOrder::Move && Order.Kind!=EOKOrder::Stance)
        return Party.IsValidIndex(ActiveMember) && Party[ActiveMember]->IsSelected() && Party[ActiveMember]->Submit(Order,Append);
    bool Accepted=false;
    int32 Index=0;
    const int32 Count=Party.FilterByPredicate([](const auto& U){ return U->IsSelected() && U->IsAlive(); }).Num();
    for (AOKRTUnit* Unit:Party)
        if (Unit->IsSelected() && Unit->IsAlive())
        {
            auto Individual=Order;
            if (Count>1 && Order.Kind==EOKOrder::Move) Individual.Location.Y+=(Index++==0 ? -45 : 45);
            const bool Promoted=Order.Kind==EOKOrder::Move && Order.bRunToDestination &&
                (Append || bTacticalPause || Unit->QueueSize()==1) && Unit->PromoteLastMoveToRun(Individual.Location);
            Accepted=Promoted || Unit->Submit(Individual,Append) || Accepted;
        }
    return Accepted;
}
void AOKRTGameMode::TogglePause()
{
    if (bInitialPausePending || bWon || bLost || bMenu) return;
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
    CameraTarget=bObjectiveComplete || bBridgeDestroyed ? ExitLocation :
        Mission().Goal==EOKMissionGoal::Documents || !bHasTNT ? TNTLocation : DetonatorLocation;
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
    if (Mission().Goal==EOKMissionGoal::Documents && !bObjectiveComplete &&
        (Kind==EOKInteraction::Nearby || Kind==EOKInteraction::CollectDocuments) && FVector::Dist2D(Unit->Feet(),TNTLocation)<140)
    { bObjectiveComplete=true; if (TNTMarker) TNTMarker->SetActorHiddenInGame(true); Message=TEXT("Dokumenty ziskane. Presunte cely tim k vychodu."); return; }
    if (Mission().Goal==EOKMissionGoal::CommandPost && bHasTNT && !bObjectiveComplete &&
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
        Mission().Goal!=EOKMissionGoal::Documents && !bHasTNT && FVector::Dist2D(Unit->Feet(),TNTLocation)<140)
    { bHasTNT=true; if (TNTMarker) TNTMarker->SetActorHiddenInGame(true); Message=TEXT("TNT ziskane."); return; }
    if ((Kind==EOKInteraction::Nearby || Kind==EOKInteraction::DetonateBridge) &&
        Mission().Goal==EOKMissionGoal::Bridge && bHasTNT && !bBridgeDestroyed && FVector::Dist2D(Unit->Feet(),DetonatorLocation)<140)
    {
        if (Party.IsEmpty() || PartyMembersOnSafeBank()!=Party.Num())
        { Message=TEXT("Najprv presunte cely tim na vychodny breh."); return; }
        bBridgeDestroyed=true; Bridge->DestroySpan();
        TArray<UStaticMeshComponent*> Parts; Bridge->GetComponents(Parts);
        for (auto* Part:Parts) { Part->SetCollisionProfileName(TEXT("NoCollision")); Part->SetCanEverAffectNavigation(false); }
        BridgeFloor->SetCollisionProfileName(TEXT("NoCollision"));
        BridgeFloor->SetCanEverAffectNavigation(false);
        FNavigationSystem::UpdateComponentData(*BridgeFloor);
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
    PreviewSpline->ClearSplinePoints(false);
    if (!bPathPreview || bMenu || !Party.IsValidIndex(ActiveMember)) return;
    auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Party[ActiveMember]->Feet(),Destination,Party[ActiveMember]);
    if (Path && Path->IsValid() && !Path->IsPartial())
        for (const auto& Point:Path->PathPoints) PreviewSpline->AddSplinePoint(Point+FVector(0,0,8),ESplineCoordinateSpace::World,false);
    PreviewSpline->UpdateSpline();
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
    if (Mission().Goal==EOKMissionGoal::Documents) return bObjectiveComplete ? TEXT("Ustupte s dokumentmi a celym timom") : TEXT("Ziskajte dokumenty z tabora");
    if (Mission().Goal==EOKMissionGoal::CommandPost)
        return bObjectiveComplete ? TEXT("Dosiahnite vychod s oboma clenmi") : !bHasTNT ? TEXT("Ziskajte TNT pri sklade") : TEXT("Vyradte velitelstvo");
    if (!bHasTNT) return TEXT("Ziskajte TNT");
    if (bBridgeDestroyed) return TEXT("Dosiahnite vychod s oboma clenmi");
    const int32 SafeMembers=PartyMembersOnSafeBank();
    return SafeMembers==Party.Num() && !Party.IsEmpty() ? TEXT("Aktivujte detonator") :
        FString::Printf(TEXT("Prejdite most: %d/%d v bezpeci"),SafeMembers,Party.Num());
}
void AOKRTGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    if (bInitialPausePending && !UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(GetWorld()) &&
        HasCompletePath(Party[0]->Feet(),DetonatorLocation) && HasCompletePath(Party[0]->Feet(),ExitLocation))
    {
        bInitialPausePending=false;
        for (AOKRTUnit* Guard:Enemies)
        {
            CastChecked<AOKRTGuardController>(Guard->GetController())->bBrainEnabled=!bSmoke || bMissionSmoke || bCampaignSmoke;
            Guard->Vision->Refresh();
        }
        TogglePause();
        if (bFrontEnd) OpenMenuPage(EOKMenuPage::Pause);
    }
    if (!bTacticalPause && !bMenu)
    {
        for (auto& Pulse:NoisePulses) Pulse.Age+=Delta;
        NoisePulses.RemoveAll([](const auto& P){ return P.Age>1.5f; });
        bool AllExited=Mission().Goal==EOKMissionGoal::Bridge ? bBridgeDestroyed : bObjectiveComplete;
        for (AOKRTUnit* Unit:Party)
        {
            if (Unit->Feet().Z<-220 && Unit->IsAlive()) Unit->TakeHit(100);
            if (!Unit->IsAlive()) bLost=true;
            AllExited=AllExited && Unit->IsAlive() && FVector::Dist2D(Unit->Feet(),ExitLocation)<160;
        }
        if (AllExited && !bLost && !bWon)
        { bWon=true; CompletedMissions|=1<<MissionId; SavePreferences(); }
        if ((bWon || bLost) && !UGameplayStatics::IsGamePaused(GetWorld()))
        { UGameplayStatics::SetGamePaused(GetWorld(),true); }
    }
    if (bCampaignSmoke) CampaignSmokeTick();
    else if (bMissionSmoke) MissionSmokeTick(Delta);
    else if (bSmoke) SmokeTick();
}
