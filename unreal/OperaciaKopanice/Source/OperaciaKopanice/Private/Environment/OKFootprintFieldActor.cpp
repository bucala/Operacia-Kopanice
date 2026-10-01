#include "Environment/OKFootprintFieldActor.h"
#include "Environment/OKFootprintTrackerSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "LandscapeProxy.h"

AOKFootprintFieldActor::AOKFootprintFieldActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.15f;
}

void AOKFootprintFieldActor::BeginPlay()
{
    Super::BeginPlay();
    if (Landscape && DepthTarget)
    {
        Landscape->SetLandscapeMaterialTextureParameterValue(TEXT("FootprintDepth"), DepthTarget);
        const FVector Origin = GetActorLocation();
        Landscape->SetLandscapeMaterialVectorParameterValue(TEXT("FootprintField"),
            FLinearColor(Origin.X, Origin.Y, FMath::Max(100.0f, FieldSizeCm), 0));
    }
    RefreshFootprints();
}

void AOKFootprintFieldActor::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); RefreshFootprints(); }

void AOKFootprintFieldActor::RefreshFootprints()
{
    if (!DepthTarget || !StampMaterial || !GetWorld() || IsRunningDedicatedServer()) return;
    UKismetRenderingLibrary::ClearRenderTarget2D(this, DepthTarget, FLinearColor::Black);
    UCanvas* Canvas = nullptr;
    FVector2D Size;
    FDrawToRenderTargetContext Context;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, DepthTarget, Canvas, Size, Context);
    if (Canvas)
    {
        const FVector Origin = GetActorLocation();
        const float Extent = FMath::Max(100.0f, FieldSizeCm);
        for (const auto& Track : GetWorld()->GetSubsystem<UOKFootprintTrackerSubsystem>()->GetActiveFootprints())
        {
            const FVector2D UV((Track.Location.X-Origin.X)/Extent, (Track.Location.Y-Origin.Y)/Extent);
            if (UV.X < 0 || UV.Y < 0 || UV.X > 1 || UV.Y > 1) continue;
            const FVector2D StampSize = Size * FVector2D(34, 16) / Extent;
            Canvas->K2_DrawMaterial(StampMaterial, UV * Size - StampSize * 0.5f, StampSize,
                FVector2D::ZeroVector, FVector2D::UnitVector, Track.Rotation.Yaw);
        }
    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}
