#include "Environment/OKFootprintTrackerSubsystem.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UOKFootprintTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UOKTurnCoordinatorSubsystem>();
    GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>()->OnReset.AddDynamic(this, &ThisClass::ClearFootprints);
}

void UOKFootprintTrackerSubsystem::Deinitialize()
{
    if (auto* Turns = GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>())
        Turns->OnReset.RemoveDynamic(this, &ThisClass::ClearFootprints);
    Footprints.Reset();
    Super::Deinitialize();
}

void UOKFootprintTrackerSubsystem::ClearFootprints() { Footprints.Reset(); }

void UOKFootprintTrackerSubsystem::RegisterFootprint(AActor* SourceActor, FVector Location,
    EOKTerrainSurface Surface, int32 LifetimeTurns, bool bDetectableByAI, FRotator Rotation)
{
    if (!GetWorld() || LifetimeTurns <= 0 || Location.ContainsNaN()) return;
    const int32 Turn = GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>()->TurnNumber;
    Footprints.RemoveAll([Turn](const FOKTrackedFootprint& F) { return F.ExpireTurn <= Turn; });
    // Bound memory even in a long uncommitted action. Oldest tracks leave first.
    if (Footprints.Num() >= 4096) Footprints.RemoveAt(0);
    FOKTrackedFootprint Footprint;
    Footprint.Location = Location;
    Footprint.Rotation = Rotation;
    Footprint.Surface = Surface;
    Footprint.SourceActor = SourceActor;
    if (SourceActor)
        if (const auto* Entity = SourceActor->FindComponentByClass<UOKEntityStateComponent>())
            Footprint.SourceFaction = Entity->Faction;
    Footprint.SpawnTurn = Turn;
    Footprint.ExpireTurn = Turn + FMath::Clamp(LifetimeTurns, 1, 10000);
    Footprint.bDetectableByAI = bDetectableByAI;
    Footprints.Add(Footprint);
}

TArray<FOKTrackedFootprint> UOKFootprintTrackerSubsystem::GetActiveFootprints() const
{
    const int32 Turn = GetWorld()->GetSubsystem<UOKTurnCoordinatorSubsystem>()->TurnNumber;
    return Footprints.FilterByPredicate([Turn](const FOKTrackedFootprint& F) { return F.ExpireTurn > Turn; });
}

TArray<FOKTrackedFootprint> UOKFootprintTrackerSubsystem::QueryDetectableFootprints(FVector Center, float Radius) const
{
    TArray<FOKTrackedFootprint> Results;
    if (Radius <= 0) return Results;
    const double RadiusSquared = FMath::Square(Radius);
    for (const FOKTrackedFootprint& F : GetActiveFootprints())
        if (F.bDetectableByAI && FVector::DistSquared(Center, F.Location) <= RadiusSquared)
            Results.Add(F);
    return Results;
}
