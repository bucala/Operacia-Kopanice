#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTHUD.h"
#include "RealTime/OKRTPlayerController.h"
#include "RealTime/OKRTUnit.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void AOKRTGameMode::SmokeMarkerTests()
{
    auto* PC=CastChecked<AOKRTPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* UI=CastChecked<AOKRTHUD>(PC->GetHUD());
    FVector2D First=FVector2D::ZeroVector,Second=FVector2D::ZeroVector;
    Require(UI->FindPartyMarker(0,First) && UI->FindPartyMarker(1,Second) &&
        UI->PartyMarkerAt(First)==0 && UI->PartyMarkerAt(Second)==1,
        TEXT("both living allies have distinct selectable screen markers"));
    if (SmokeStage<0) return;
    const int32 Queue0=Party[0]->QueueSize(),Queue1=Party[1]->QueueSize();
    auto Click=[PC](FVector2D P,bool Append=false) { PC->BeginPointer(P,Append); PC->EndPointer(P); };
    FVector Origin=FVector::ZeroVector,Direction=FVector::ZeroVector;
    Require(PC->DeprojectScreenPositionToWorld(First.X,First.Y,Origin,Direction),TEXT("marker fixture deprojects its real pointer ray"));
    if (SmokeStage<0) return;
    // An opaque render mesh blocks the real pick ray. Only the friendly HUD
    // affordance bypasses it; neither world collision nor AI vision is changed.
    auto* Obstacle=Place(TEXT("/Engine/BasicShapes/Cube.Cube"),Origin+Direction*1200,4);
    auto* Mesh=Obstacle->GetStaticMeshComponent();
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    FHitResult Hit;
    Require(PC->GetHitResultAtScreenPosition(First,ECC_Visibility,false,Hit) && Hit.GetActor()==Obstacle,
        TEXT("opaque geometry occludes the underlying unit pick ray"));
    Select(1); Click(First);
    Require(ActiveMember==0 && Party[0]->IsSelected() && !Party[1]->IsSelected(),
        TEXT("marker selects an occluded ally without a terrain ray hit"));
    Click(Second,true);
    Require(ActiveMember==1 && Party[0]->IsSelected() && Party[1]->IsSelected(),
        TEXT("Shift marker click adds the second specialist"));
    Require(!PC->CommandAt(First) && !PC->CommandAt(First,true) && ActiveMember==1,
        TEXT("right click and double click on a marker never issue world orders"));
    PC->Arm(EOKOrder::Distract); Click(First);
    Require(PC->bArmed && ActiveMember==1,TEXT("friendly marker cannot consume an armed ability"));
    Require(PC->CommandAt(First) && !PC->bArmed,TEXT("right click on a marker still cancels armed targeting"));
    Require(PC->TapAt(First) && ActiveMember==0 && !Party[1]->IsSelected(),
        TEXT("touch marker tap selects instead of moving or interacting"));
    Require(Party[0]->QueueSize()==Queue0 && Party[1]->QueueSize()==Queue1 &&
        bTacticalPause && UGameplayStatics::IsGamePaused(GetWorld()),
        TEXT("marker input preserves queued orders and true tactical pause"));
    Obstacle->Destroy();

    FVector2D Unused;
    Party[0]->Health=0;
    Require(!UI->FindPartyMarker(0,Unused),TEXT("dead allies have no selectable world marker"));
    Party[0]->Health=100; Party[0]->SetActorHiddenInGame(true);
    Require(!UI->FindPartyMarker(0,Unused),TEXT("hidden allies have no selectable world marker"));
    Party[0]->SetActorHiddenInGame(false);
    Require(!UI->FindPartyMarker(Party.Num(),Unused),TEXT("party markers never expose enemy actors"));
    bPartyMarkers=false;
    Require(!UI->FindPartyMarker(0,Unused) && UI->PartyMarkerAt(First)==INDEX_NONE,
        TEXT("disabled party markers immediately remove their hit targets"));
    bPartyMarkers=true;
    ToggleMenu();
    Require(!UI->FindPartyMarker(0,Unused) && !PC->CommandAt(First),TEXT("menus suppress party markers and world commands"));
    ToggleMenu();

    const FVector Saved=Party[1]->GetActorLocation();
    Party[1]->SetActorLocation(Party[0]->GetActorLocation());
    Require(UI->FindPartyMarker(0,First) && UI->FindPartyMarker(1,Second) &&
        FVector2D::Distance(First,Second)>=32 && UI->PartyMarkerAt(First)==0 && UI->PartyMarkerAt(Second)==1,
        TEXT("overlapping allies receive separated marker hit regions"));
    Party[1]->SetActorLocation(Camera->GetActorLocation()-Camera->GetActorForwardVector()*500);
    Require(!UI->FindPartyMarker(1,Unused),TEXT("offscreen allies do not leave invisible marker hit regions"));
    Party[1]->SetActorLocation(Saved);
    FVector Head; FRotator Facing; Party[1]->GetActorEyesViewPoint(Head,Facing);
    const FVector HeadOffset=Head+FVector(0,0,35)-Saved;
    PC->DeprojectScreenPositionToWorld(25,30,Origin,Direction);
    Party[1]->SetActorLocation(Origin+Direction*2500-HeadOffset);
    Require(!UI->FindPartyMarker(1,Unused),TEXT("world markers cannot overlap a portrait panel"));
    Party[1]->SetActorLocation(Saved);

    UI->FindPartyMarker(0,First);
    const float Yaw=CameraYaw,Tilt=CameraTilt,Distance=CameraDistance;
    Orbit(45,0,-250); PC->PlayerCameraManager->UpdateCamera(0);
    Require(UI->FindPartyMarker(0,Second) && !Second.Equals(First,1) && UI->PartyMarkerAt(Second)==0,
        TEXT("paused camera orbit and zoom refresh marker drawing and hit geometry"));
    CameraYaw=Yaw; CameraTilt=Tilt; CameraDistance=Distance; UpdateCamera();
    PC->PlayerCameraManager->UpdateCamera(0);
    bWon=true;
    Require(!UI->FindPartyMarker(0,Unused),TEXT("results screens suppress friendly world markers"));
    bWon=false;
    SelectAll();
}
