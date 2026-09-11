#include "Misc/AutomationTest.h"
#include "Environment/OKTerrainSurfaceTypes.h"
#include "Environment/OKTerrainWeightMap.h"
#include "Environment/OKPCGScatterRuleSet.h"
#include "ECS/OKTurnCoordinatorSubsystem.h"
#include "AI/OKWeatherPerceptionComponent.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOKTerrainTest, "OperaciaKopanice.Terrain.Blending", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOKTerrainTest::RunTest(const FString& Parameters)
{
    const auto* Data = NewObject<UOKTerrainSurfaceDataAsset>();
    const auto Mixed = Data->BlendRuntime(FVector(0, 0.5, 0.5));
    TestTrue(TEXT("Friction blends"), FMath::IsNearlyEqual(Mixed.Friction, 0.45f, 0.001f));
    TestTrue(TEXT("Speed blends"), FMath::IsNearlyEqual(Mixed.SpeedModifier, 0.3925f, 0.001f));
    TestEqual(TEXT("Empty weights fall back to paved"), Data->BlendRuntime(FVector::ZeroVector).SpeedModifier, 1.0f);
    TestEqual(TEXT("Null physical material is paved"), Data->GetRuntimeForPhysicalMaterial(nullptr).Surface, EOKTerrainSurface::PavedRoadsInteriors);
    TestEqual(TEXT("Pure snow speed"), Data->BlendRuntime(FVector(0, 0, 5)).SpeedModifier, 0.285f);
    auto* Map = NewObject<UOKTerrainWeightMap>();
    Map->Weights = {FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), FVector(0,0,1)};
    FVector Weight;
    TestTrue(TEXT("Sample center"), Map->Sample(FVector(5000,5000,0), Weight));
    TestTrue(TEXT("Bilinear interpolation"), Weight.Equals(FVector(0.25,0.25,0.5)));
    TestFalse(TEXT("Outside field has no override"), Map->Sample(FVector(-1,0,0), Weight));
    Map->Weights.Pop();
    TestFalse(TEXT("Malformed map rejected"), Map->Sample(FVector::ZeroVector, Weight));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOKScatterTest, "OperaciaKopanice.PCG.Determinism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOKScatterTest::RunTest(const FString& Parameters)
{
    const auto* Rules = NewObject<UOKPCGScatterRuleSet>();
    const FTransform Input(FRotator(15, 0, 0), FVector(100,200,0));
    FTransform A, B;
    int32 Accepted = 0;
    for (int32 Seed = 0; Seed < 100; ++Seed)
    {
        const bool First = Rules->EvaluatePoint(0, Input, 1, Seed, 400, A);
        const bool Second = Rules->EvaluatePoint(0, Input, 1, Seed, 400, B);
        TestEqual(TEXT("Seed acceptance stable"), First, Second);
        TestTrue(TEXT("Transforms stable"), A.Equals(B));
        if (First) { ++Accepted; TestTrue(TEXT("Beech grows upright"), A.GetRotation().GetUpVector().Equals(FVector::UpVector)); }
    }
    TestTrue(TEXT("Clumps produce a nonempty subset"), Accepted > 0 && Accepted < 100);
    TestFalse(TEXT("Reject sea-level stand"), Rules->EvaluatePoint(0, Input, 1, 1, 0, A));
    TestFalse(TEXT("Reject invalid rule"), Rules->EvaluatePoint(9, Input, 1, 1, 400, A));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOKTurnsTest, "OperaciaKopanice.Turns.PhaseGuards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOKTurnsTest::RunTest(const FString& Parameters)
{
    auto* World = NewObject<UWorld>();
    auto* Turns = NewObject<UOKTurnCoordinatorSubsystem>(World);
    TestFalse(TEXT("Cannot finish enemy phase before commit"), Turns->CompleteEnemyReaction());
    TestTrue(TEXT("Commit accepted"), Turns->CommitPlayerAction());
    TestFalse(TEXT("Duplicate action rejected"), Turns->CommitPlayerAction());
    TestEqual(TEXT("One action one turn"), Turns->TurnNumber, 1);
    TestTrue(TEXT("Enemy completion"), Turns->CompleteEnemyReaction());
    TestTrue(TEXT("Outcome completion"), Turns->CompleteOutcomeEvaluation());
    Turns->ResetTurns();
    TestEqual(TEXT("Reset turn"), Turns->TurnNumber, 0);
    TestEqual(TEXT("Reset phase"), Turns->Phase, EOKTurnPhase::PlayerAction);
    auto* Weather = NewObject<UOKWeatherPerceptionComponent>();
    Weather->ApplyWeatherState(EOKWeatherVisibilityState::AutumnFog, 1, 0);
    TestEqual(TEXT("Fog sight reduction"), Weather->GetSightRangeMultiplier(), 0.42f);
    Weather->ApplyWeatherState(EOKWeatherVisibilityState::Clear, 0, 0);
    TestEqual(TEXT("Clear restores range without compounding"), Weather->GetSightRangeMultiplier(), 1.0f);
    return true;
}
#endif
