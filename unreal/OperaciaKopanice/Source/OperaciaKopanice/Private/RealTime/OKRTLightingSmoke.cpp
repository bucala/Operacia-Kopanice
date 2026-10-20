#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTPlayerController.h"
#include "Navigation/OKViaductActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Scalability.h"
#include "UnrealClient.h"

void AOKRTGameMode::LightingSmokeTick()
{
    if (SmokeStage<0 || bInitialPausePending) return;
    const double Now=FPlatformTime::Seconds();
    if (Now-SmokeStarted>150) { Require(false,TEXT("lighting fixture timeout")); return; }
    auto Next=[&]() { if (SmokeStage>=0) { ++SmokeStage; StageStarted=Now; } };
    auto Capture=[&](const TCHAR* View)
    {
        FScreenshotRequest::RequestScreenshot(*FString::Printf(TEXT("OKRT_Lighting_Q%d_%s.png"),Quality,View),false,false);
    };
    if (SmokeStage==0)
    {
        auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
        int32 W,H; PC->GetViewportSize(W,H); PC->SetMouseLocation(W-2,H-2);
        TArray<AOKRTUnit*> Units; for (const auto& U:Party) Units.Add(U); for (const auto& U:Enemies) Units.Add(U);
        for (const auto* U:Units)
        {
            const TCHAR* MaterialRole=U->Specialist==EOKSpecialist::Officer ? TEXT("Officer") : TEXT("Partisan");
            auto* Expected=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Kopanice/Supplied/%s/M_%s.M_%s"),MaterialRole,MaterialRole,MaterialRole));
            Require(Expected && U->AnimatedVisual->GetNumMaterials()>0,TEXT("animated unit has canonical textured material"));
            for (int32 Slot=0;Slot<U->AnimatedVisual->GetNumMaterials();++Slot)
                Require(U->AnimatedVisual->GetMaterial(Slot)==Expected,TEXT("player and enemy render uniform instead of WorldGrid placeholder"));
        }
        int32 Suns=0,Skies=0;
        for (TActorIterator<ADirectionalLight> It(GetWorld());It;++It)
        {
            ++Suns;
            const auto* Light=CastChecked<UDirectionalLightComponent>(It->GetLightComponent());
            Require(Light->Mobility==EComponentMobility::Movable && Light->CastShadows &&
                FMath::IsNearlyEqual(Light->LightSourceAngle,2.f),TEXT("one movable sun has contact-preserving soft dynamic shadows"));
        }
        for (TActorIterator<ASkyLight> It(GetWorld());It;++It)
        {
            ++Skies;
            const auto* Light=It->GetLightComponent();
            Require(!Light->bRealTimeCapture && Light->SourceType==SLS_SpecifiedCubemap && Light->Cubemap &&
                Light->LowerHemisphereColor.GetLuminance()>.3f && Light->CastShadows,
                TEXT("deterministic daylight probe and snow hemisphere provide shadowed ambient light"));
        }
        Require(Suns==1 && Skies==1,TEXT("lighting has no competing directional fill light"));
        Require(bTacticalPause && UGameplayStatics::IsGamePaused(GetWorld()),
            TEXT("lighting review runs with paused simulation and a live renderer"));
        Require(Bridge && Bridge->Tags.Contains(TEXT("RustboundBridge")) && Bridge->IsDemoModelConsistent() &&
            FMath::IsNearlyEqual(Bridge->GetActorLocation().Z,BridgeFloor->Bounds.GetBox().Max.Z,.1),
            TEXT("supplied bridge deck is visible and aligned with bank-height navigation"));
        Quality=0;
        Scalability::FQualityLevels Levels; Levels.SetFromSingleQualityLevel(Quality);
        Scalability::SetQualityLevels(Levels);
        CameraTarget=FVector(900,720,0); CameraYaw=58.86f; CameraTilt=47; CameraDistance=4250;
        UpdateCamera(); Next(); return;
    }
    if (SmokeStage==17)
    {
        if (Now-StageStarted<3) return;
        Capture(TEXT("Bridge")); Next(); return;
    }
    if (SmokeStage==18)
    {
        if (Now-StageStarted<.5) return;
        Require(HasCompletePath(Party[0]->Feet(),DetonatorLocation) && HasCompletePath(Party[0]->Feet(),ExitLocation),
            TEXT("lighting and bridge changes preserve mission navigation"));
        if (SmokeStage<0) return;
        UE_LOG(LogTemp,Display,TEXT("OK_RT_LIGHTING: PASS"));
        SmokeStage=-1;
        FPlatformMisc::RequestExit(false); return;
    }
    const int32 Stage=(SmokeStage-1)%4;
    if (Stage==0)
    {
        if (Now-StageStarted<4) return; // Allow Nanite and GI histories to settle at each quality.
        const auto& PP=Camera->GetCameraComponent()->PostProcessSettings;
        Require(PP.AutoExposureMethod==AEM_Manual && !PP.AutoExposureApplyPhysicalCameraExposure &&
            PP.AmbientOcclusionIntensity<.5f && PP.MotionBlurAmount==0,
            TEXT("quality changes retain stable exposure, local AO and blur-free pause"));
        Capture(TEXT("Overview")); Next(); return;
    }
    if (Stage==1)
    {
        if (Now-StageStarted<.5) return;
        CameraTarget=FVector(540,850,0); CameraDistance=2200; UpdateCamera();
        Next(); return;
    }
    if (Stage==2)
    {
        if (Now-StageStarted<3) return;
        Capture(TEXT("Close")); Next(); return;
    }
    if (Now-StageStarted<.5) return;
    if (Quality<3)
    {
        // Exercise the actual options path, but never overwrite player preferences.
        CycleQuality();
        CameraTarget=FVector(900,720,0); CameraDistance=4250;
        CameraYaw=Quality==3 ? 148.86f : 58.86f; UpdateCamera();
        Next(); return;
    }
    CameraTarget=FVector(1080,720,0); CameraDistance=1700; CameraYaw=58.86f; UpdateCamera();
    Next();
}
