#include "RealTime/OKRTGameMode.h"
#include "RealTime/OKRTUnit.h"
#include "RealTime/OKRTGuardController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"

void AOKRTGameMode::SmokeNavigationTests()
{
    auto* Unit=Party[0].Get();
    auto* Guard=Enemies[0].Get();
    auto* AI=CastChecked<AOKRTGuardController>(Unit->GetController());
    const FTransform UnitPose=Unit->GetActorTransform(),GuardPose=Guard->GetActorTransform();
    FOKRTOrder Move; Move.Location=FVector(180,960,0);
    Require(Unit->Submit(Move),TEXT("reachable movement is accepted before navigation checks"));
    Unit->Tick(0);
    const auto Request=AI->GetPathFollowingComponent()->GetCurrentRequestId();
    Require(AI->GetMoveStatus()==EPathFollowingStatus::Moving,TEXT("navigation rejection fixture has an active path"));
    FOKRTOrder Water; Water.Location=FVector(1080,1260,0);
    Require(!Unit->Submit(Water) && Unit->QueueSize()==1 &&
        AI->GetPathFollowingComponent()->GetCurrentRequestId()==Request,
        TEXT("river click is rejected without cancelling current movement"));
    FOKRTOrder Cabin; Cabin.Location=FVector(450,270,0);
    Require(!Unit->Submit(Cabin) && Unit->QueueSize()==1,
        TEXT("cabin interior without navigation cannot replace a move"));
    FOKRTOrder Approach=Cabin; Approach.Kind=EOKOrder::Interact; Approach.bApproachInteraction=true;
    Require(!Unit->Submit(Approach) && Unit->QueueSize()==1,
        TEXT("unreachable interaction leaves the current order intact"));
    FOKRTOrder Shore; Shore.Location=FVector(990,1260,0);
    Require(!Unit->Submit(Shore) && Unit->QueueSize()==1,
        TEXT("off-mesh shore click is not silently redirected onto a bank"));
    FOKRTOrder Above; Above.Location=FVector(720,1260,500);
    Require(!Unit->Submit(Above) && Unit->QueueSize()==1,
        TEXT("unreachable elevated target is not projected onto the floor"));
    Guard->SetActorLocation(FVector(1080,1260,90));
    FOKRTOrder Attack; Attack.Kind=EOKOrder::Takedown; Attack.Target=Guard;
    Require(!Unit->Submit(Attack) && Unit->QueueSize()==1,
        TEXT("unreachable guard approach cannot cancel a valid move"));
    Guard->SetActorTransform(GuardPose); Unit->CancelOrders();
    Select(0);
    FOKRTOrder NearBank; NearBank.Location=FVector(920,1260,0);
    Require(Unit->Submit(NearBank),TEXT("near-bank valid move is available for double-click regression"));
    FOKRTOrder DoubleClick; DoubleClick.Location=FVector(970,1260,0); DoubleClick.bRunToDestination=true;
    Require(!Command(DoubleClick) && Unit->QueueSize()==1,
        TEXT("off-mesh double-click cannot bypass validation by promoting a nearby move"));
    Unit->CancelOrders();
    TogglePause();
    FOKRTOrder Across; Across.Location=FVector(1440,720,0);
    Require(Unit->Submit(Across) && Unit->Submit(Move,true) && Unit->QueueSize()==2,
        TEXT("paused navigation accepts connected routes from queued destinations"));
    Require(!Unit->Submit(Water,true) && Unit->QueueSize()==2,
        TEXT("invalid paused append preserves the planned route"));
    Unit->CancelOrders();
    FOKRTOrder AlreadyThere; AlreadyThere.Location=Unit->Feet();
    Require(Unit->Submit(AlreadyThere),TEXT("same-position movement remains a valid no-op"));
    Unit->CancelOrders();
    Require(Unit->Submit(Attack),TEXT("reachable guard can be targeted while paused"));
    FOKRTOrder Stance; Stance.Kind=EOKOrder::Stance; Stance.Stance=EOKStance::Crouch;
    Unit->Submit(Stance,true);
    Guard->SetActorLocation(FVector(1080,1260,90)); TogglePause(); Unit->Tick(0);
    Require(Unit->QueueSize()==1 && AI->GetMoveStatus()==EPathFollowingStatus::Idle && Guard->IsAlive(),
        TEXT("resume rejects a now-unreachable approach without discarding later orders"));
    Unit->Tick(0);
    Require(Unit->QueueSize()==0 && Unit->Stance==EOKStance::Crouch,
        TEXT("valid order following rejected approach still executes"));
    Unit->SetStance(EOKStance::Walk); Guard->SetActorTransform(GuardPose);

    // A visibility-only wall keeps navigation stable and isolates the hand-reach check.
    auto* WallActor=GetWorld()->SpawnActor<AActor>(FVector(765,1260,130),FRotator::ZeroRotator);
    auto* Wall=NewObject<UBoxComponent>(WallActor);
    WallActor->SetRootComponent(Wall); WallActor->AddInstanceComponent(Wall);
    Wall->SetBoxExtent(FVector(5,120,150)); Wall->SetCanEverAffectNavigation(false);
    Wall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
    Wall->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Wall->RegisterComponent(); Wall->SetWorldLocation(FVector(765,1260,130));
    Unit->SetActorLocation(FVector(720,1260,90));
    Guard->SetActorLocation(FVector(810,1260,90)); Guard->Health=0;
    FOKRTOrder Carry; Carry.Kind=EOKOrder::Carry; Carry.Target=Guard;
    const FName Profile=Guard->GetCapsuleComponent()->GetCollisionProfileName();
    const auto Collision=Guard->GetCapsuleComponent()->GetCollisionEnabled();
    const auto Responses=Guard->GetCapsuleComponent()->GetCollisionResponseToChannels();
    Require(Unit->Submit(Carry),TEXT("body reach fixture accepts its complete navigation path"));
    Unit->Tick(0);
    Require(Unit->QueueSize()==0 && !Unit->CarriedBody.IsValid() && !Guard->Carrier.IsValid(),
        TEXT("body pickup cannot reach through a visibility-blocking wall"));
    WallActor->Destroy();
    Require(Unit->Submit(Carry),TEXT("body pickup can be retried after the obstruction is removed"));
    Unit->Tick(0);
    Require(Unit->CarriedBody==Guard && Guard->Carrier==Unit,
        TEXT("unobstructed body pickup still succeeds"));
    Unit->DropBody();
    Guard->GetCapsuleComponent()->SetCollisionProfileName(Profile);
    Guard->GetCapsuleComponent()->SetCollisionEnabled(Collision);
    Guard->GetCapsuleComponent()->SetCollisionResponseToChannels(Responses);
    Guard->Health=100; Guard->SetActorTransform(GuardPose);
    Unit->CancelOrders(); Unit->SetActorTransform(UnitPose); Unit->SetStance(EOKStance::Walk);
}
