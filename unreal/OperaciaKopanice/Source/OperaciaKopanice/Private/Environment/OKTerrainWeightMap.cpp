#include "Environment/OKTerrainWeightMap.h"

bool UOKTerrainWeightMap::Sample(FVector WorldLocation, FVector& OutWeights) const
{
    OutWeights = FVector(1, 0, 0);
    if (Resolution.X < 2 || Resolution.Y < 2 ||
        int64(Resolution.X) * Resolution.Y != Weights.Num() ||
        WorldSizeCm.X <= 0 || WorldSizeCm.Y <= 0) return false;
    const FVector2D UV = (FVector2D(WorldLocation.X, WorldLocation.Y) - WorldOriginCm) / WorldSizeCm;
    if (UV.X < 0 || UV.Y < 0 || UV.X > 1 || UV.Y > 1) return false;
    const double X = UV.X * (Resolution.X - 1), Y = UV.Y * (Resolution.Y - 1);
    const int32 X0 = FMath::Min(FMath::FloorToInt(X), Resolution.X - 2);
    const int32 Y0 = FMath::Min(FMath::FloorToInt(Y), Resolution.Y - 2);
    const auto At = [this](int32 I, int32 J) { return Weights[J * Resolution.X + I]; };
    OutWeights = FMath::Lerp(FMath::Lerp(At(X0, Y0), At(X0+1, Y0), X-X0),
        FMath::Lerp(At(X0, Y0+1), At(X0+1, Y0+1), X-X0), Y-Y0);
    return true;
}
