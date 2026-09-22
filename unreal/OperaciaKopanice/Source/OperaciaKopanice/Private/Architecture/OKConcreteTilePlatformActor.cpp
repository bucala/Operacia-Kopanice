#include "Architecture/OKConcreteTilePlatformActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"

AOKConcreteTilePlatformActor::AOKConcreteTilePlatformActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConcreteTile"));
	TileMesh->SetupAttachment(SceneRoot);
	TileMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));

	LeftEngravingText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LeftSideEngraving"));
	LeftEngravingText->SetupAttachment(SceneRoot);

	RightEngravingText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RightSideEngraving"));
	RightEngravingText->SetupAttachment(SceneRoot);

	RearEngravingText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RearEngraving"));
	RearEngravingText->SetupAttachment(SceneRoot);
}

void AOKConcreteTilePlatformActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEngravings();
}

void AOKConcreteTilePlatformActor::RefreshEngravings()
{
	TileDimensions = TileDimensions.ComponentMax(FVector(100, 100, 10));
	if (TileMesh)
	{
		TileMesh->SetRelativeScale3D(TileDimensions / 100.0f);
		if (ConcreteMaterial)
		{
			TileMesh->SetMaterial(0, ConcreteMaterial);
		}
	}

	const float HalfX = TileDimensions.X * 0.5f + 1.0f;
	const float HalfY = TileDimensions.Y * 0.5f + 1.0f;
	const float Z = 0.0f;

	const auto ConfigureText = [this](UTextRenderComponent* Text, const FString& Value)
	{
		if (!Text)
		{
			return;
		}
		Text->SetText(FText::FromString(Value));
		Text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Text->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		const float SideLength = Text == RearEngravingText ? TileDimensions.Y : TileDimensions.X;
		const float FitSize = (SideLength - 24.0f) / FMath::Max(1.0f, Value.Len() * 0.7f);
		Text->SetWorldSize(FMath::Min3(18.0f, float(TileDimensions.Z) * 0.6f, FitSize));
		Text->SetTextRenderColor(FColor(34, 34, 34));
		if (EngravingTextMaterial)
		{
			Text->SetMaterial(0, EngravingTextMaterial);
		}
	};

	ConfigureText(LeftEngravingText, SideEngraving);
	LeftEngravingText->SetRelativeLocation(FVector(0.0f, -HalfY, Z));
	LeftEngravingText->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	ConfigureText(RightEngravingText, SideEngraving);
	RightEngravingText->SetRelativeLocation(FVector(0.0f, HalfY, Z));
	RightEngravingText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	ConfigureText(RearEngravingText, RearEngraving);
	RearEngravingText->SetRelativeLocation(FVector(-HalfX, 0.0f, Z));
	RearEngravingText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}
