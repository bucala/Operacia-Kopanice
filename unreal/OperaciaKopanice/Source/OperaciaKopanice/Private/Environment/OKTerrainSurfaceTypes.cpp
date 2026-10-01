#include "Environment/OKTerrainSurfaceTypes.h"

UOKTerrainSurfaceDataAsset::UOKTerrainSurfaceDataAsset()
{
	FOKTerrainSurfaceRuntime Paved;
	Paved.Surface = EOKTerrainSurface::PavedRoadsInteriors;
	Paved.LandscapeLayerName = TEXT("Paved_Roads_Interiors");
	Paved.Friction = 1.0f;
	Paved.SpeedModifier = 1.0f;
	Paved.bSpawnFootprints = false;

	FOKTerrainSurfaceRuntime Mud;
	Mud.Surface = EOKTerrainSurface::AutumnMud;
	Mud.LandscapeLayerName = TEXT("Autumn_Mud");
	Mud.Friction = 0.6f;
	Mud.SpeedModifier = 0.5f;
	Mud.bSpawnFootprints = true;
	Mud.bFootprintsAreTemporary = true;
	Mud.FootprintLifetimeSeconds = 22.0f;

	FOKTerrainSurfaceRuntime Snow;
	Snow.Surface = EOKTerrainSurface::DeepWinterSnow;
	Snow.LandscapeLayerName = TEXT("Deep_Winter_Snow");
	Snow.Friction = 0.3f;
	Snow.SpeedModifier = 0.285f;
	Snow.bSpawnFootprints = true;
	Snow.bFootprintsAreTemporary = false;
	Snow.bFootprintsDetectableByAI = true;
	Snow.FootprintLifetimeSeconds = 180.0f;
	Snow.AISnowTrackSenseRadius = 1200.0f;

	Surfaces = { Paved, Mud, Snow };
}

FOKTerrainSurfaceRuntime UOKTerrainSurfaceDataAsset::GetRuntimeForSurface(EOKTerrainSurface Surface) const
{
	for (const FOKTerrainSurfaceRuntime& Candidate : Surfaces)
	{
		if (Candidate.Surface == Surface)
		{
			return Candidate;
		}
	}

	return Surfaces.Num() > 0 ? Surfaces[0] : FOKTerrainSurfaceRuntime();
}

FOKTerrainSurfaceRuntime UOKTerrainSurfaceDataAsset::GetRuntimeForPhysicalMaterial(const UPhysicalMaterial* PhysicalMaterial) const
{
	for (const FOKTerrainSurfaceRuntime& Candidate : Surfaces)
	{
		if (PhysicalMaterial && Candidate.PhysicalMaterial == PhysicalMaterial)
		{
			return Candidate;
		}
	}

	return GetRuntimeForSurface(EOKTerrainSurface::PavedRoadsInteriors);
}

FOKTerrainSurfaceRuntime UOKTerrainSurfaceDataAsset::BlendRuntime(FVector Weights) const
{
	Weights.X = FMath::Max(0.0f, Weights.X);
	Weights.Y = FMath::Max(0.0f, Weights.Y);
	Weights.Z = FMath::Max(0.0f, Weights.Z);
	const double Sum = Weights.X + Weights.Y + Weights.Z;
	if (Sum <= SMALL_NUMBER) Weights = FVector(1, 0, 0);
	else Weights /= Sum;
	const FOKTerrainSurfaceRuntime Paved = GetRuntimeForSurface(EOKTerrainSurface::PavedRoadsInteriors);
	const FOKTerrainSurfaceRuntime Mud = GetRuntimeForSurface(EOKTerrainSurface::AutumnMud);
	const FOKTerrainSurfaceRuntime Snow = GetRuntimeForSurface(EOKTerrainSurface::DeepWinterSnow);
	FOKTerrainSurfaceRuntime Result = Weights.Z > Weights.X && Weights.Z > Weights.Y ? Snow :
		(Weights.Y > Weights.X ? Mud : Paved);
	Result.Friction = Paved.Friction * Weights.X + Mud.Friction * Weights.Y + Snow.Friction * Weights.Z;
	Result.SpeedModifier = Paved.SpeedModifier * Weights.X + Mud.SpeedModifier * Weights.Y + Snow.SpeedModifier * Weights.Z;
	return Result;
}
