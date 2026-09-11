#include "Environment/OKPCGScatterRuleSet.h"

UOKPCGScatterRuleSet::UOKPCGScatterRuleSet()
{
	FOKPCGScatterRule Beech;
	Beech.Prototype = EOKScatterPrototype::EuropeanBeech;
	Beech.DensityPer1000SqM = 62.0f;
	Beech.MinSlopeDegrees = 4.0f;
	Beech.MaxSlopeDegrees = 34.0f;
	Beech.MinAltitude = 280.0f;
	Beech.MaxAltitude = 900.0f;
	Beech.UniformScaleRange = FVector2D(0.75f, 1.45f);

	FOKPCGScatterRule Rock;
	Rock.Prototype = EOKScatterPrototype::LimestoneVrsatecRock;
	Rock.DensityPer1000SqM = 9.0f;
	Rock.MinSlopeDegrees = 22.0f;
	Rock.MaxSlopeDegrees = 58.0f;
	Rock.MinAltitude = 390.0f;
	Rock.MaxAltitude = 970.0f;
	Rock.UniformScaleRange = FVector2D(0.55f, 2.8f);
	Rock.bAlignToNormal = true;

	Rules = { Beech, Rock };
}

bool UOKPCGScatterRuleSet::EvaluatePoint(int32 RuleIndex, FTransform SurfaceTransform, float InputDensity,
	int32 PointSeed, float AltitudeOffsetMeters, FTransform& OutTransform) const
{
	OutTransform = SurfaceTransform;
	if (!Rules.IsValidIndex(RuleIndex)) return false;
	const auto& Rule = Rules[RuleIndex];
	const FVector Position = SurfaceTransform.GetLocation();
	const FVector Normal = SurfaceTransform.GetRotation().GetUpVector();
	const double Slope = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Normal.Z, -1.0, 1.0)));
	const double Altitude = Position.Z / 100.0 + AltitudeOffsetMeters;
	if (Slope < Rule.MinSlopeDegrees || Slope > Rule.MaxSlopeDegrees ||
		Altitude < Rule.MinAltitude || Altitude > Rule.MaxAltitude) return false;
	FRandomStream Random(PointSeed);
	const float Clump = 0.35f + 0.65f * FMath::Clamp(0.5f + FMath::PerlinNoise2D(FVector2D(Position.X, Position.Y) / 4000.0f), 0.0f, 1.0f);
	if (Random.FRand() >= FMath::Clamp(InputDensity, 0.0f, 1.0f) * Clump) return false;
	const FQuat Alignment = Rule.bAlignToNormal ? FQuat::FindBetweenNormals(FVector::UpVector, Normal) : FQuat::Identity;
	OutTransform.SetRotation(Alignment * FQuat(FVector::UpVector, Random.FRandRange(0.0f, 2.0f * PI)));
	const float Scale = Random.FRandRange(FMath::Max(0.01, Rule.UniformScaleRange.X),
		FMath::Max(FMath::Max(0.01, Rule.UniformScaleRange.X), Rule.UniformScaleRange.Y));
	OutTransform.SetScale3D(FVector(Scale));
	return true;
}
