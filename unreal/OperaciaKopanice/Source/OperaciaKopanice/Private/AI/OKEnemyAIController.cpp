#include "AI/OKEnemyAIController.h"
#include "AI/OKTurnPerceptionBridgeSubsystem.h"
#include "AI/OKWeatherPerceptionComponent.h"
#include "Environment/OKFootprintTrackerSubsystem.h"
#include "ECS/OKEntityComponents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"

AOKEnemyAIController::AOKEnemyAIController()
{
    Senses = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
    SetPerceptionComponent(*Senses);
    auto* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
    auto* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
    Sight->DetectionByAffiliation.bDetectEnemies = true;
    Sight->DetectionByAffiliation.bDetectNeutrals = true;
    Sight->DetectionByAffiliation.bDetectFriendlies = true;
    Hearing->DetectionByAffiliation = Sight->DetectionByAffiliation;
    Sight->SetMaxAge(5.0f);
    Hearing->SetMaxAge(5.0f);
    Senses->ConfigureSense(*Sight);
    Senses->ConfigureSense(*Hearing);
    Senses->SetDominantSense(UAISense_Sight::StaticClass());
    Weather = CreateDefaultSubobject<UOKWeatherPerceptionComponent>(TEXT("WeatherPerception"));
}

void AOKEnemyAIController::BeginPlay()
{
    Super::BeginPlay();
    Weather->PushToAIPerceptionComponent();
}

bool AOKEnemyAIController::CanSeeLocation(FVector Location, AActor* Target) const
{
    if (!GetPawn()) return false;
    FVector Eye; FRotator Rotation;
    GetPawn()->GetActorEyesViewPoint(Eye, Rotation);
    const FVector Delta = Location - Eye;
    if (Delta.SizeSquared() > FMath::Square(Weather->BaseSightRange * Weather->GetSightRangeMultiplier())) return false;
    const float HalfAngle = Weather->BaseSightHalfAngle * FMath::Lerp(0.65f, 1.0f, Weather->GetSightRangeMultiplier());
    if (FVector::DotProduct(Rotation.Vector(), Delta.GetSafeNormal()) < FMath::Cos(FMath::DegreesToRadians(HalfAngle))) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OKTurnSight), false, GetPawn());
    if (Target) Params.AddIgnoredActor(Target);
    return !GetWorld()->LineTraceTestByChannel(Eye, Location, ECC_Visibility, Params);
}

void AOKEnemyAIController::CaptureReaction(const TArray<FOKTurnNoise>& Sounds, int32 TurnNumber)
{
    Reaction = FOKEnemyReaction();
    Reaction.TurnNumber = TurnNumber;
    if (!GetPawn()) return;
    if (const auto* Self = GetPawn()->FindComponentByClass<UOKEntityStateComponent>())
        if (Self->Health <= 0) return;
    const FVector Origin = GetPawn()->GetActorLocation();
    double Nearest = TNumericLimits<double>::Max();
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        const auto* Entity = It->FindComponentByClass<UOKEntityStateComponent>();
        if (!Entity || Entity->Faction != EOKFaction::Partisan || Entity->Health <= 0) continue;
        FVector Eye; FRotator Rotation;
        It->GetActorEyesViewPoint(Eye, Rotation);
        const double Distance = FVector::DistSquared(Origin, It->GetActorLocation());
        const bool TieWins = Distance == Nearest && Reaction.Target && It->GetPathName() < Reaction.Target->GetPathName();
        if ((Distance < Nearest || TieWins) && CanSeeLocation(Eye, *It))
        {
            Nearest = Distance;
            Reaction.Clue = EOKReactionClue::Sight;
            Reaction.Target = *It;
            Reaction.Location = It->GetActorLocation();
        }
    }
    if (Reaction.Clue != EOKReactionClue::None) return;
    for (const FOKTurnNoise& Sound : Sounds)
    {
        if (Sound.Faction == EOKFaction::Volksgrenadier708) continue;
        FVector Eye; FRotator Rotation;
        GetPawn()->GetActorEyesViewPoint(Eye, Rotation);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(OKSoundOcclusion), false, GetPawn());
        if (Sound.Instigator.IsValid()) Params.AddIgnoredActor(Sound.Instigator.Get());
        const bool Occluded = GetWorld()->LineTraceTestByChannel(Eye, Sound.Location, ECC_Visibility, Params);
        const float Range = FMath::Min(Sound.MaxRange, Weather->BaseHearingRange * Weather->GetHearingRangeMultiplier()) *
            Sound.Loudness * (Occluded ? FMath::Clamp(OccludedSoundMultiplier, 0.0f, 1.0f) : 1.0f);
        const double Distance = FVector::DistSquared(Origin, Sound.Location);
        if (Distance <= FMath::Square(Range) && Distance < Nearest)
        {
            Nearest = Distance;
            Reaction.Clue = EOKReactionClue::Sound;
            Reaction.Location = Sound.Location;
        }
    }
    if (Reaction.Clue != EOKReactionClue::None) return;
    const auto Tracks = GetWorld()->GetSubsystem<UOKFootprintTrackerSubsystem>()->QueryDetectableFootprints(Origin, TrackSearchRadius);
    // Newest visible hostile clue first. A radius query alone must not see through walls.
    for (int32 I = Tracks.Num() - 1; I >= 0; --I)
    {
        const auto& Track = Tracks[I];
        if (Track.SourceFaction != EOKFaction::Partisan || !CanSeeLocation(Track.Location + FVector(0, 0, 4))) continue;
        Reaction.Clue = EOKReactionClue::SnowTrack;
        Reaction.Location = Track.Location;
        break;
    }
}

void AOKEnemyAIController::DispatchReaction()
{
    if (GetPawn()) OnTurnReaction(Reaction);
}
