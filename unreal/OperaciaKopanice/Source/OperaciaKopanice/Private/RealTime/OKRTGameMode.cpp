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
    bSmoke=FParse::Param(FCommandLine::Get(),TEXT("OKRTSmoke"));
    SmokeStarted=StageStarted=FPlatformTime::Seconds();
    BuildScene();
    Message=TEXT("Infiltracia zacala.");
}
void AOKRTGameMode::BuildScene()
{
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1500),FRotator(-55,-40,0));
    Sun->GetLightComponent()->SetIntensity(5.2f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.96f,.89f));
    auto* SunComponent=CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
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
        const FVector Location=I==0 ? FVector(180,1260,100) : I==1 ? FVector(360,1260,100) : I==2 ? FVector(720,720,100) : FVector(1620,900,100);
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Unit=GetWorld()->SpawnActor<AOKRTUnit>(Location,FRotator(0,Enemy ? 90 : 0,0),Params);
        Unit->Initialize(Enemy,I!=0);
        if (Enemy)
        {
            Enemies.Add(Unit);
            auto* AI=CastChecked<AOKRTGuardController>(Unit->GetController());
            AI->PatrolRoute=I==2 ? TArray<FVector>{FVector(720,720,0),FVector(720,1080,0),FVector(540,1080,0),FVector(540,720,0)} :
                TArray<FVector>{FVector(1620,900,0),FVector(1620,1260,0),FVector(1440,1260,0),FVector(1440,900,0)};
            AI->bBrainEnabled=false;
        }
        else Party.Add(Unit);
    }
    Select(0);
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
    auto* Bounds=GetWorld()->SpawnActor<AOKRTNavBoundsVolume>(FVector(900,720,0),FRotator::ZeroRotator);
    if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    { Nav->OnNavigationBoundsUpdated(Bounds); Nav->Build(); }
}
void AOKRTGameMode::Select(int32 Index,bool Append)
{
    if (!Party.IsValidIndex(Index)) return;
    if (!Append) for (AOKRTUnit* Unit:Party) Unit->Select(false);
    Party[Index]->Select(true); ActiveMember=Index;
    PreviewSpline->ClearSplinePoints();
}
void AOKRTGameMode::SelectAll()
{
    for (AOKRTUnit* Unit:Party) Unit->Select(true);
}
bool AOKRTGameMode::Command(FOKRTOrder Order,bool Append)
{
    if (bMenu || bWon || bLost) return false;
    // Movement and stance affect the selection; scarce skills belong to the active portrait.
    if (Order.Kind!=EOKOrder::Move && Order.Kind!=EOKOrder::Stance)
        return Party.IsValidIndex(ActiveMember) && Party[ActiveMember]->IsSelected() && Party[ActiveMember]->Submit(Order,Append);
    bool Accepted=false;
    int32 Index=0;
    const int32 Count=Party.FilterByPredicate([](const auto& U){ return U->IsSelected(); }).Num();
    for (AOKRTUnit* Unit:Party)
        if (Unit->IsSelected() && Unit->IsAlive())
        {
            auto Individual=Order;
            if (Count>1 && Order.Kind==EOKOrder::Move) Individual.Location.Y+=(Index++==0 ? -45 : 45);
            Accepted=Unit->Submit(Individual,Append) || Accepted;
        }
    return Accepted;
}
void AOKRTGameMode::TogglePause()
{
    if (bWon || bLost || bMenu) return;
    bTacticalPause=!bTacticalPause;
    UGameplayStatics::SetGamePaused(GetWorld(),bTacticalPause);
    Message=bTacticalPause ? TEXT("Takticka pauza") : TEXT("Prikazy vykonavane.");
}
void AOKRTGameMode::ToggleMenu()
{
    bMenu=!bMenu; bOptions=false;
    UGameplayStatics::SetGamePaused(GetWorld(),bMenu || bTacticalPause);
}
void AOKRTGameMode::Restart()
{
    UGameplayStatics::SetGamePaused(GetWorld(),false);
    UGameplayStatics::OpenLevel(GetWorld(),TEXT("/Engine/Maps/Entry"));
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
    CameraDistance=FMath::Clamp(CameraDistance+Zoom,1700.f,6500.f); UpdateCamera();
}
void AOKRTGameMode::Pan(float X,float Y)
{
    const FVector Right=Camera->GetActorRightVector().GetSafeNormal2D();
    const FVector Forward=Camera->GetActorForwardVector().GetSafeNormal2D();
    CameraTarget+=Right*X+Forward*Y;
    CameraTarget.X=FMath::Clamp(CameraTarget.X,-300.,2200.);
    CameraTarget.Y=FMath::Clamp(CameraTarget.Y,-300.,2000.); UpdateCamera();
}
void AOKRTGameMode::FocusSelected()
{
    if (Party.IsValidIndex(ActiveMember)) CameraTarget=Party[ActiveMember]->Feet();
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
void AOKRTGameMode::Interact(AOKRTUnit* Unit)
{
    if (Unit->CarriedBody.IsValid() && FVector::Dist2D(Unit->Feet(),HideLocation)<170)
    { Unit->DropBody(true); Message=TEXT("Telo ukryte."); return; }
    if (!bHasTNT && FVector::Dist2D(Unit->Feet(),TNTLocation)<140)
    { bHasTNT=true; if (TNTMarker) TNTMarker->SetActorHiddenInGame(true); Message=TEXT("TNT ziskane."); return; }
    if (bHasTNT && !bBridgeDestroyed && FVector::Dist2D(Unit->Feet(),DetonatorLocation)<140)
    {
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
bool AOKRTGameMode::IsCover(FVector Location) const
{
    return FVector::Dist2D(Location,FVector(600,1440,0))<170;
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
FString AOKRTGameMode::Objective() const
{
    if (bLost) return TEXT("Misia zlyhala");
    if (bWon) return TEXT("Misia splnena");
    return !bHasTNT ? TEXT("Ziskajte TNT") : !bBridgeDestroyed ? TEXT("Prejdite most a aktivujte detonator") : TEXT("Dosiahnite vychod s oboma clenmi");
}
void AOKRTGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    if (bInitialPausePending && HasCompletePath(Party[0]->Feet(),DetonatorLocation))
    {
        bInitialPausePending=false;
        for (AOKRTUnit* Guard:Enemies)
        {
            CastChecked<AOKRTGuardController>(Guard->GetController())->bBrainEnabled=!bSmoke;
            Guard->Vision->Refresh();
        }
        TogglePause();
    }
    if (!bTacticalPause && !bMenu)
    {
        for (auto& Pulse:NoisePulses) Pulse.Age+=Delta;
        NoisePulses.RemoveAll([](const auto& P){ return P.Age>1.5f; });
        bool AllExited=bBridgeDestroyed;
        for (AOKRTUnit* Unit:Party)
        {
            if (Unit->Feet().Z<-220 && Unit->IsAlive()) Unit->TakeHit(100);
            if (!Unit->IsAlive()) bLost=true;
            AllExited=AllExited && Unit->IsAlive() && FVector::Dist2D(Unit->Feet(),ExitLocation)<160;
        }
        if (AllExited && !bLost) bWon=true;
        if ((bWon || bLost) && !UGameplayStatics::IsGamePaused(GetWorld()))
        { UGameplayStatics::SetGamePaused(GetWorld(),true); }
    }
    if (bSmoke) SmokeTick();
}
