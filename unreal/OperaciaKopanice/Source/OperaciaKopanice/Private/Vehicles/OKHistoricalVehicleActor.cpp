#include "Vehicles/OKHistoricalVehicleActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

AOKHistoricalVehicleActor::AOKHistoricalVehicleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetMobility(EComponentMobility::Static);
}

void AOKHistoricalVehicleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MeshComponent->SetStaticMesh(VehicleMesh);
	if (BodyMaterial) MeshComponent->SetMaterial(0, BodyMaterial);
	if (GlassMaterial && GlassMaterialSlot < MeshComponent->GetNumMaterials()) MeshComponent->SetMaterial(GlassMaterialSlot, GlassMaterial);
	UpdateWeatheringMaterialParameters();
}

void AOKHistoricalVehicleActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateWeatheringMaterialParameters();
}

void AOKHistoricalVehicleActor::ApplyVehiclePreset()
{
	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(VehicleMesh);
		if (BodyMaterial)
		{
			MeshComponent->SetMaterial(0, BodyMaterial);
		}
		if (GlassMaterial && GlassMaterialSlot < MeshComponent->GetNumMaterials()) MeshComponent->SetMaterial(GlassMaterialSlot, GlassMaterial);
	}

	switch (VehicleType)
	{
	case EOKHistoricalVehicleType::KubelwagenTyp82:
		MudAmount = 0.34f;
		FrostAmount = 0.28f;
		RoofSnowAmount = 0.18f;
		PaintWearAmount = 0.22f;
		break;
	case EOKHistoricalVehicleType::OpelBlitz3T:
		MudAmount = 0.42f;
		FrostAmount = 0.2f;
		RoofSnowAmount = 0.34f;
		PaintWearAmount = 0.26f;
		break;
	case EOKHistoricalVehicleType::TatraT77A1938:
	default:
		MudAmount = 0.16f;
		FrostAmount = 0.36f;
		RoofSnowAmount = 0.2f;
		PaintWearAmount = 0.08f;
		break;
	}

	UpdateWeatheringMaterialParameters();
}

void AOKHistoricalVehicleActor::UpdateWeatheringMaterialParameters()
{
	DynamicMaterials.Reset();
	if (!MeshComponent)
	{
		return;
	}

	const int32 MaterialCount = MeshComponent->GetNumMaterials();
	for (int32 Index = 0; Index < MaterialCount; ++Index)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(Index));
		if (!DynamicMaterial) DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(Index);
		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetScalarParameterValue(TEXT("MudAmount"), MudAmount);
		DynamicMaterial->SetScalarParameterValue(TEXT("FrostAmount"), FrostAmount);
		DynamicMaterial->SetScalarParameterValue(TEXT("RoofSnowAmount"), RoofSnowAmount);
		DynamicMaterial->SetScalarParameterValue(TEXT("PaintWearAmount"), PaintWearAmount);
		DynamicMaterial->SetScalarParameterValue(TEXT("UseHeightSnowMask"), 1.0f);
		DynamicMaterials.Add(DynamicMaterial);
	}
}
