#include "Architecture/OKModularBuildingTemplateActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	if (bWinterDetails) AddWinterDetails(Width, Depth, StonePlinthHeight);
}

void AOKModularBuildingTemplateActor::AddWinterDetails(float Width, float Depth, float PlinthHeight)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto ColorMaterial = [this, Base](FLinearColor Color)
	{
		auto* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		return Material;
	};
	auto* Timber = ColorMaterial(FLinearColor(0.16f,0.12f,0.085f));
	auto* Trim = ColorMaterial(FLinearColor(0.30f,0.25f,0.18f));
	auto* Snow = ColorMaterial(FLinearColor(0.83f,0.90f,0.95f));
	auto* Glass = ColorMaterial(FLinearColor(0.12f,0.23f,0.27f));
	auto* Iron = ColorMaterial(FLinearColor(0.07f,0.08f,0.08f));
	FRandomStream Random(1944);
	auto Part = [this](FVector P, FVector S, UMaterialInterface* M)
	{
		return AddBoxPart(TEXT("WinterDetail"), P, S, M);
	};
	// Distinct stone blocks, staggered at the corners of the foundation.
	for (int32 Row=0; Row<2; ++Row)
	{
		const int32 Count = FMath::Max(3, FMath::CeilToInt(Width/65.f));
		const float Step = Width/Count;
		for (int32 I=0; I<Count; ++I)
		{
			const float Shade = Random.FRandRange(0.22f,0.37f);
			auto* Stone = ColorMaterial(FLinearColor(Shade,Shade*1.03f,Shade*1.06f));
			for (float Side : {-1.f,1.f})
				Part(FVector(-Width/2+(I+0.5f)*Step,Side*(Depth/2+3),PlinthHeight*(Row+0.5f)/2),
					FVector(Step-3,18,PlinthHeight/2-3),Stone);
		}
	}
	if (Style != EOKModularBuildingStyle::CarpathianStonework)
	{
		const int32 Courses = FMath::CeilToInt(WallHeight/22.f);
		const float CourseHeight = WallHeight/Courses;
		for (int32 Row=0; Row<Courses; ++Row)
		{
			const float Z=PlinthHeight+(Row+0.5f)*CourseHeight;
			const float Extension=(Row%2==0 ? 18.f : 8.f);
			for (float Side : {-1.f,1.f})
				Part(FVector(Side*(Width/2+5),0,Z),FVector(26,Depth+Extension,CourseHeight-2),Timber);
			Part(FVector(0,-Depth/2-6,Z),FVector(Width+Extension,26,CourseHeight-2),Timber);
			const float Segment=(Width-104)/2;
			for (float Side : {-1.f,1.f})
				Part(FVector(Side*(52+Segment/2),Depth/2+6,Z),FVector(Segment,26,CourseHeight-2),Timber);
		}
	}
	// Door boards and hinges stay inside the existing doorway.
	for (int32 I=0; I<6; ++I)
		Part(FVector(-42.5f+I*17,Depth/2+8,PlinthHeight+102),FVector(16,10,204),Trim);
	for (float Z : {45.f,155.f})
		Part(FVector(-25,Depth/2+15,PlinthHeight+Z),FVector(44,4,6),Iron);
	Part(FVector(32,Depth/2+16,PlinthHeight+95),FVector(5,8,12),Iron);
	// Side windows are shallow glazed recess representations, not traversable openings.
	for (float Side : {-1.f,1.f})
	{
		const FVector C(Side*(Width/2+21),0,PlinthHeight+140);
		Part(C,FVector(5,86,92),Trim);
		Part(C+FVector(Side*4,0,0),FVector(4,70,76),Glass);
		Part(C+FVector(Side*8,0,0),FVector(5,5,80),Trim);
		Part(C+FVector(Side*8,0,0),FVector(5,74,5),Trim);
		Part(C+FVector(0,0,-49),FVector(28,102,9),Trim);
		Part(C+FVector(0,0,-42),FVector(29,102,6),Snow);
	}
	const float Run=Width/2+43;
	const float Pitch=FMath::DegreesToRadians(35.f);
	const float Eave=PlinthHeight+WallHeight;
	// Individual gable boards close the triangular void below the roof.
	const int32 Boards=FMath::CeilToInt(Width/20.f);
	for (int32 I=0; I<Boards; ++I)
	{
		const float X=-Width/2+(I+0.5f)*Width/Boards;
		const float H=(Run-FMath::Abs(X))*FMath::Tan(Pitch);
		for (float Side : {-1.f,1.f})
			Part(FVector(X,Side*(Depth/2+8),Eave+H/2),FVector(Width/Boards-1,18,H),Timber);
	}
	for (float Side : {-1.f,1.f})
	{
		auto* Cap=Part(FVector(Side*Run/2,0,Eave+Run/2*FMath::Tan(Pitch)+18),
			FVector(Run/FMath::Cos(Pitch)+10,Depth+98,18),Snow);
		Cap->SetRelativeRotation(FRotator(-Side*35,0,0));
		for (float End : {-1.f,1.f})
		{
			auto* Fascia=Part(FVector(Side*Run/2,End*(Depth/2+44),Eave+Run/2*FMath::Tan(Pitch)),
				FVector(Run/FMath::Cos(Pitch)+8,12,24),Trim);
			Fascia->SetRelativeRotation(FRotator(-Side*35,0,0));
		}
	}
	const float ChimneyX=Width*0.22f;
	const float RoofAtChimney=Eave+(Run-ChimneyX)*FMath::Tan(Pitch);
	Part(FVector(ChimneyX,-Depth*0.24f,RoofAtChimney+30),FVector(42,46,110),Trim);
	Part(FVector(ChimneyX,-Depth*0.24f,RoofAtChimney+86),FVector(56,60,10),Snow);
	Part(FVector(ChimneyX,-Depth*0.24f,RoofAtChimney+92),FVector(25,29,3),Iron);
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
	Component->SetMobility(SceneRoot->Mobility);
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
