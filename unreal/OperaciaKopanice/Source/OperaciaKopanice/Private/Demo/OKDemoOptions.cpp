#include "Demo/OKDemoGameMode.h"
#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AOKDemoGameMode::SelectLevel(int32 Index)
{
    State=OKDemo::InitialState(Index);
    MenuPage=0;
    Restart();
    ResetCamera();
}
void AOKDemoGameMode::ToggleMenu()
{
    MenuPage=MenuPage==0 ? 1 : MenuPage==1 ? 0 : 1;
}
void AOKDemoGameMode::SaveOptions()
{
    // Automated tests must never overwrite the player's preferences.
    if (FParse::Param(FCommandLine::Get(),TEXT("OKDemoSmoke"))) return;
    GConfig->SetBool(TEXT("Kopanice.Options"),TEXT("Grid"),bGridVisible,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Kopanice.Options"),TEXT("Camera"),bCameraEnabled,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Kopanice.Options"),TEXT("Stepped"),bSteppedRotation,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Kopanice.Options"),TEXT("Quality"),Quality,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void AOKDemoGameMode::ToggleGrid()
{
    bGridVisible=!bGridVisible;
    RefreshScene();
    SaveOptions();
}
void AOKDemoGameMode::ToggleCamera() { bCameraEnabled=!bCameraEnabled; SaveOptions(); }
void AOKDemoGameMode::ToggleRotationMode() { bSteppedRotation=!bSteppedRotation; SaveOptions(); }
void AOKDemoGameMode::CycleQuality()
{
    Quality=(Quality+1)%4;
    if (auto* Settings=GEngine->GetGameUserSettings())
    {
        Settings->SetOverallScalabilityLevel(Quality);
        Settings->ApplyNonResolutionSettings();
        Settings->SaveSettings();
    }
    SaveOptions();
}
void AOKDemoGameMode::UpdateCamera()
{
    if (!Camera) return;
    const FVector Target(900,720,0);
    const float Yaw=FMath::DegreesToRadians(CameraYaw);
    const float Tilt=FMath::DegreesToRadians(CameraTilt);
    const FVector Offset(FMath::Cos(Yaw)*FMath::Cos(Tilt),
        FMath::Sin(Yaw)*FMath::Cos(Tilt),FMath::Sin(Tilt));
    Camera->SetActorLocation(Target+Offset*CameraDistance);
    Camera->SetActorRotation((-Offset).Rotation());
}
void AOKDemoGameMode::ResetCamera()
{
    CameraYaw=58.86f; CameraTilt=51.87f; CameraDistance=4450.f;
    UpdateCamera();
}
void AOKDemoGameMode::Orbit(float YawDelta,float TiltDelta,float ZoomDelta)
{
    if (!bCameraEnabled || MenuPage!=0) return;
    if (YawDelta==0 && TiltDelta==0 && ZoomDelta==0) return;
    CameraYaw=FMath::UnwindDegrees(CameraYaw+YawDelta);
    CameraTilt=FMath::Clamp(CameraTilt+TiltDelta,35.f,75.f);
    CameraDistance=FMath::Clamp(CameraDistance+ZoomDelta,2900.f,6200.f);
    UpdateCamera();
}
