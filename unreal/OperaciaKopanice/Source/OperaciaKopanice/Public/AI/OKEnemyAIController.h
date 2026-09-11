#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "OKEnemyAIController.generated.h"

class UOKWeatherPerceptionComponent;
class UAIPerceptionComponent;
struct FOKTurnNoise;

UENUM(BlueprintType)
enum class EOKReactionClue : uint8 { None, Sight, Sound, SnowTrack };

USTRUCT(BlueprintType)
struct FOKEnemyReaction
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="AI")
    EOKReactionClue Clue = EOKReactionClue::None;
    UPROPERTY(BlueprintReadOnly, Category="AI")
    FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="AI")
    TObjectPtr<AActor> Target = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="AI")
    int32 TurnNumber = 0;
};

UCLASS()
class OPERACIAKOPANICE_API AOKEnemyAIController : public AAIController
{
    GENERATED_BODY()
public:
    AOKEnemyAIController();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    TObjectPtr<UAIPerceptionComponent> Senses;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    TObjectPtr<UOKWeatherPerceptionComponent> Weather;
    UPROPERTY(BlueprintReadOnly, Category="AI")
    FOKEnemyReaction Reaction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    float TrackSearchRadius = 1200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI", meta=(ClampMin="0", ClampMax="1"))
    float OccludedSoundMultiplier = 0.35f;

    void CaptureReaction(const TArray<FOKTurnNoise>& Sounds, int32 TurnNumber);
    void DispatchReaction();
    UFUNCTION(BlueprintImplementableEvent, Category="Turns")
    void OnTurnReaction(const FOKEnemyReaction& Observation);
protected:
    virtual void BeginPlay() override;
private:
    bool CanSeeLocation(FVector Location, AActor* Target = nullptr) const;
};
