#include "AI/OKTurnPerceptionBridgeSubsystem.h"

#include "Perception/AISense_Hearing.h"
#include "AI/OKEnemyAIController.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void UOKTurnPerceptionBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UOKTurnCoordinatorSubsystem>();
	auto* Turns = GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>();
	Turns->OnPhaseChanged.AddDynamic(this, &ThisClass::HandlePhase);
	Turns->OnReset.AddDynamic(this, &ThisClass::ResetSounds);
}

void UOKTurnPerceptionBridgeSubsystem::Deinitialize()
{
	if (auto* Turns = GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>())
	{
		Turns->OnPhaseChanged.RemoveDynamic(this, &ThisClass::HandlePhase);
		Turns->OnReset.RemoveDynamic(this, &ThisClass::ResetSounds);
	}
	PendingSounds.Reset();
	Super::Deinitialize();
}

void UOKTurnPerceptionBridgeSubsystem::ResetSounds() { PendingSounds.Reset(); }

void UOKTurnPerceptionBridgeSubsystem::ReportTurnSound(AActor* InstigatorActor, FVector Location, float Loudness, FName Tag)
{
	if (!GetWorld() || !IsValid(InstigatorActor) || InstigatorActor->GetWorld() != GetWorld() ||
		!FMath::IsFinite(Loudness) || Loudness <= 0 || Location.ContainsNaN() ||
		GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>()->Phase != EOKTurnPhase::PlayerAction)
	{
		return;
	}

	FOKTurnNoise Noise;
	Noise.Location = Location;
	Noise.Instigator = InstigatorActor;
	Noise.Loudness = FMath::Clamp(Loudness, 0.0f, 1.0f);
	Noise.MaxRange = FMath::Max(0.0f, DefaultNoiseMaxRange);
	if (const auto* Entity = InstigatorActor->FindComponentByClass<UOKEntityStateComponent>()) Noise.Faction = Entity->Faction;
	if (PendingSounds.Num() < 256) PendingSounds.Add(Noise);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Location, Noise.Loudness, InstigatorActor, Noise.MaxRange, Tag);
}

void UOKTurnPerceptionBridgeSubsystem::HandlePhase(EOKTurnPhase Phase, int32 TurnNumber)
{
	if (Phase != EOKTurnPhase::EnemyReaction) return;
	const TArray<FOKTurnNoise> Sounds = MoveTemp(PendingSounds);
	PendingSounds.Reset();
	TArray<AOKEnemyAIController*> Controllers;
	for (TActorIterator<AOKEnemyAIController> It(GetWorld()); It; ++It) Controllers.Add(*It);
	Controllers.Sort([](const AOKEnemyAIController& A, const AOKEnemyAIController& B)
	{
		return A.GetPathName() < B.GetPathName();
	});
	// Freeze every observation before dispatching any gameplay reaction.
	for (AOKEnemyAIController* Controller : Controllers) Controller->CaptureReaction(Sounds, TurnNumber);
	for (AOKEnemyAIController* Controller : Controllers)
		if (IsValid(Controller)) Controller->DispatchReaction();
}
