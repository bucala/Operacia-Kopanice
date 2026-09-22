#include "Environment/OKTerrainMovementSurfaceComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"
#include "LandscapeProxy.h"
#include "Environment/OKTerrainWeightMap.h"
#include "Environment/OKFootprintTrackerSubsystem.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"

UOKTerrainMovementSurfaceComponent::UOKTerrainMovementSurfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.08f;
	CurrentRuntime.Surface = EOKTerrainSurface::PavedRoadsInteriors;
}

void UOKTerrainMovementSurfaceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!SurfaceData) SurfaceData = NewObject<UOKTerrainSurfaceDataAsset>(this);

	if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		CachedMovement = CharacterOwner->GetCharacterMovement();
		if (CachedMovement.IsValid())
		{
			BaseWalkSpeed = CachedMovement->MaxWalkSpeed;
			BaseGroundFriction = CachedMovement->GroundFriction;
		}
	}

	SampleSurface();
	ApplyMovementModifier();
}

void UOKTerrainMovementSurfaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bApplySpeedModifierToCharacterMovement && CachedMovement.IsValid())
	{
		CachedMovement->MaxWalkSpeed = BaseWalkSpeed;
		CachedMovement->GroundFriction = BaseGroundFriction;
	}
	Super::EndPlay(EndPlayReason);
}

void UOKTerrainMovementSurfaceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SampleSurface();
}

void UOKTerrainMovementSurfaceComponent::SampleSurface()
{
	if (!GetWorld() || !GetOwner() || !SurfaceData)
	{
		return;
	}

	if (CachedMovement.IsValid() && !CachedMovement->IsMovingOnGround())
	{
		bHasGroundSample = false;
		return;
	}
	const FVector Start = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector End = Start - FVector(0.0f, 0.0f, TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OKTerrainSurfaceTrace), false, GetOwner());
	QueryParams.bReturnPhysicalMaterial = true;

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		bHasGroundSample = false;
		return;
	}

	FOKTerrainSurfaceRuntime SampledRuntime = SurfaceData->GetRuntimeForPhysicalMaterial(Hit.PhysMaterial.Get());
	FVector Weights;
	if (WeightMap && Cast<ALandscapeProxy>(Hit.GetActor()) && WeightMap->Sample(Hit.ImpactPoint, Weights))
	{
		SampledRuntime = SurfaceData->BlendRuntime(Weights);
	}
	const bool bChangedSurface = SampledRuntime.Surface != CurrentRuntime.Surface;
	CurrentRuntime = SampledRuntime;
	ApplyMovementModifier();

	if (bChangedSurface)
	{
		OnTerrainSurfaceChanged.Broadcast(CurrentRuntime.Surface, CurrentRuntime.SpeedModifier);
	}

	MaybeRequestFootprint(Hit.ImpactPoint);
}

void UOKTerrainMovementSurfaceComponent::ApplyMovementModifier() const
{
	if (!bApplySpeedModifierToCharacterMovement || !CachedMovement.IsValid())
	{
		return;
	}

	CachedMovement->MaxWalkSpeed = BaseWalkSpeed * CurrentRuntime.SpeedModifier;
	CachedMovement->GroundFriction = CurrentRuntime.Friction * BaseGroundFriction;
}

void UOKTerrainMovementSurfaceComponent::MaybeRequestFootprint(const FVector& Location)
{
	if (!bHasGroundSample || !CurrentRuntime.bSpawnFootprints)
	{
		LastFootprintLocation = Location;
		bHasGroundSample = true;
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(Location, LastFootprintLocation);
	if (DistanceSquared < FMath::Square(FMath::Max(1.0f, FootstepSpacing)))
	{
		return;
	}

	LastFootprintLocation = Location;
	if (DistanceSquared > FMath::Square(1000.0f)) return; // Teleport, not a footstep.
	const FRotator FootprintRotation(0.0f, GetOwner() ? GetOwner()->GetActorRotation().Yaw : 0.0f, 0.0f);
	if (CurrentRuntime.bFootprintsDetectableByAI)
	{
		GetWorld()->GetSubsystem<UOKFootprintTrackerSubsystem>()->RegisterFootprint(GetOwner(), Location,
			CurrentRuntime.Surface, CurrentRuntime.FootprintLifetimeTurns, true, FootprintRotation);
	}
	else if (MudDecalMaterial)
	{
		if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), MudDecalMaterial,
			FVector(12, 9, 18), Location, FRotator(-90, FootprintRotation.Yaw, 0), CurrentRuntime.FootprintLifetimeSeconds))
		{
			Decal->SetFadeOut(FMath::Max(0.0f, CurrentRuntime.FootprintLifetimeSeconds - 3.0f), 3.0f, false);
		}
	}
	OnFootprintRequested.Broadcast(Location, FootprintRotation, CurrentRuntime.Surface);
}
