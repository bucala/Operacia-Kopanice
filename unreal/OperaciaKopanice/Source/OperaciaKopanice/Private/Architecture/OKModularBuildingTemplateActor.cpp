#include "Architecture/OKModularBuildingTemplateActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"

AOKModularBuildingTemplateActor::AOKModularBuildingTemplateActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

void AOKModularBuildingTemplateActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildTemplate();
}

void AOKModularBuildingTemplateActor::RebuildTemplate()
{
	ClearGeneratedParts();
	BayCountX = FMath::Clamp(BayCountX, 1, 8);
	BayCountY = FMath::Clamp(BayCountY, 1, 6);
	BaySize = FMath::Clamp(BaySize, 180.0f, 1000.0f);
	WallHeight = FMath::Clamp(WallHeight, 220.0f, 1000.0f);

	const float Width = BayCountX * BaySize;
	const float Depth = BayCountY * BaySize;
	const float HalfWidth = Width * 0.5f;
	const float HalfDepth = Depth * 0.5f;
	const float WallThickness = 24.0f;
	const float StonePlinthHeight = Style == EOKModularBuildingStyle::DrevenicaLogCabin ? 45.0f : 90.0f;

	AddBoxPart(TEXT("StonePlinth"), FVector(0.0f, 0.0f, StonePlinthHeight * 0.5f), FVector(Width, Depth, StonePlinthHeight), StoneMaterial);
	AddBoxPart(TEXT("NorthWall"), FVector(0.0f, -HalfDepth, StonePlinthHeight + WallHeight * 0.5f), FVector(Width, WallThickness, WallHeight), LogMaterial);
	const float DoorWidth = 100.0f;
	const float DoorHeight = 210.0f;
	const float SideWidth = (Width - DoorWidth) * 0.5f;
	AddBoxPart(TEXT("DoorWallL"), FVector(-(DoorWidth + SideWidth) * 0.5f, HalfDepth, StonePlinthHeight + WallHeight * 0.5f), FVector(SideWidth, WallThickness, WallHeight), LogMaterial);
	AddBoxPart(TEXT("DoorWallR"), FVector((DoorWidth + SideWidth) * 0.5f, HalfDepth, StonePlinthHeight + WallHeight * 0.5f), FVector(SideWidth, WallThickness, WallHeight), LogMaterial);
	AddBoxPart(TEXT("DoorLintel"), FVector(0, HalfDepth, StonePlinthHeight + (WallHeight + DoorHeight) * 0.5f), FVector(DoorWidth, WallThickness, WallHeight - DoorHeight), LogMaterial);
	AddBoxPart(TEXT("WestWall"), FVector(-HalfWidth, 0.0f, StonePlinthHeight + WallHeight * 0.5f), FVector(WallThickness, Depth, WallHeight), LogMaterial);
	AddBoxPart(TEXT("EastWall"), FVector(HalfWidth, 0.0f, StonePlinthHeight + WallHeight * 0.5f), FVector(WallThickness, Depth, WallHeight), LogMaterial);

	if (Style != EOKModularBuildingStyle::CarpathianStonework)
	{
		for (int32 Row = 0; Row < FMath::FloorToInt(WallHeight / 31.0f); ++Row)
		{
			const float Z = StonePlinthHeight + 24.0f + Row * 31.0f;
			AddBoxPart(*FString::Printf(TEXT("VisibleLogCourse_%02d_N"), Row), FVector(0.0f, -HalfDepth - 9.0f, Z), FVector(Width + 38.0f, 18.0f, 18.0f), LogMaterial);
			if (Z > StonePlinthHeight + DoorHeight + 10.0f)
				AddBoxPart(*FString::Printf(TEXT("VisibleLogCourse_%02d_S"), Row), FVector(0.0f, HalfDepth + 9.0f, Z), FVector(Width + 38.0f, 18.0f, 18.0f), LogMaterial);
		}
	}

	const float Run = Width * 0.5f + 43.0f;
	const float Pitch = FMath::DegreesToRadians(35.0f);
	const float RoofZ = StonePlinthHeight + WallHeight + Run * 0.5f * FMath::Tan(Pitch);
	AddBoxPart(TEXT("LeftRoofPlane"), FVector(-Run * 0.5f, 0, RoofZ), FVector(Run / FMath::Cos(Pitch), Depth + 86, 20), RoofMaterial)->SetRelativeRotation(FRotator(35, 0, 0));
	AddBoxPart(TEXT("RightRoofPlane"), FVector(Run * 0.5f, 0, RoofZ), FVector(Run / FMath::Cos(Pitch), Depth + 86, 20), RoofMaterial)->SetRelativeRotation(FRotator(-35, 0, 0));

	if (bGeneratePorch)
	{
		AddBoxPart(TEXT("PorchDeck"), FVector(0.0f, HalfDepth + 78.0f, StonePlinthHeight + 10.0f), FVector(Width * 0.55f, 135.0f, 20.0f), LogMaterial);
		AddBoxPart(TEXT("PorchPostL"), FVector(-Width * 0.23f, HalfDepth + 132.0f, StonePlinthHeight + 110.0f), FVector(18.0f, 18.0f, 220.0f), LogMaterial);
		AddBoxPart(TEXT("PorchPostR"), FVector(Width * 0.23f, HalfDepth + 132.0f, StonePlinthHeight + 110.0f), FVector(18.0f, 18.0f, 220.0f), LogMaterial);
	}
}

void AOKModularBuildingTemplateActor::ClearGeneratedParts()
{
	for (UStaticMeshComponent* Part : GeneratedParts)
	{
		if (Part)
		{
			RemoveInstanceComponent(Part);
			Part->DestroyComponent();
		}
	}
	GeneratedParts.Reset();
}

UStaticMeshComponent* AOKModularBuildingTemplateActor::AddBoxPart(FName Name, FVector LocalLocation, FVector LocalScale, UMaterialInterface* Material)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name), RF_Transactional);
	AddInstanceComponent(Component);
	Component->SetStaticMesh(CubeMesh);
	Component->SetMobility(EComponentMobility::Static);
	Component->SetupAttachment(SceneRoot);
	Component->SetRelativeLocation(LocalLocation);
	Component->SetRelativeScale3D(LocalScale / 100.0f);
	if (Style == EOKModularBuildingStyle::CarpathianStonework && Material == LogMaterial) Material = StoneMaterial;
	if (Material)
	{
		Component->SetMaterial(0, Material);
	}
	Component->RegisterComponent();
	GeneratedParts.Add(Component);
	return Component;
}
