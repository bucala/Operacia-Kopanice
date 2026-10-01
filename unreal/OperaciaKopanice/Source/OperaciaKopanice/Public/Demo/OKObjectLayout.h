#pragma once

#include "CoreMinimal.h"

namespace OKLayout
{
    constexpr float CellSize = 180.f;
    constexpr int32 LargeMapSize = 48;
    constexpr int32 ExtendedMapSize = 96;
    enum class EObject : uint8
    {
        Cabin, House, Barn, Car, Truck, MediumTank, HeavyTank, Fighter, TransportAircraft
    };

    inline FIntPoint Footprint(EObject Object)
    {
        switch (Object)
        {
        case EObject::Cabin: return {4,4};
        case EObject::House: return {6,5};
        case EObject::Barn: return {8,5};
        case EObject::Car: return {3,2};
        case EObject::Truck: return {5,2};
        case EObject::MediumTank: return {5,3};
        case EObject::HeavyTank: return {6,3};
        case EObject::Fighter: return {7,6};
        case EObject::TransportAircraft: return {18,11};
        }
        return {0,0};
    }

    // Origin is the minimum occupied cell after rotation, not the mesh pivot.
    // Only cardinal object rotations are supported; camera yaw is independent.
    struct FPlacement
    {
        EObject Object;
        FIntPoint Origin;
        int32 QuarterTurns = 0;

        FIntPoint Size() const
        {
            const auto Base=Footprint(Object);
            return QuarterTurns%2 == 0 ? Base : FIntPoint(Base.Y,Base.X);
        }
        bool Contains(FIntPoint Cell) const
        {
            const auto S=Size();
            return Cell.X>=Origin.X && Cell.Y>=Origin.Y &&
                int64(Cell.X)-Origin.X<S.X && int64(Cell.Y)-Origin.Y<S.Y;
        }
        bool Fits(FIntPoint MapSize) const
        {
            const auto S=Size();
            return S.X>0 && S.Y>0 && Origin.X>=0 && Origin.Y>=0 &&
                int64(Origin.X)+S.X<=MapSize.X && int64(Origin.Y)+S.Y<=MapSize.Y;
        }
        bool Overlaps(const FPlacement& Other) const
        {
            const auto A=Size(), B=Other.Size();
            return A.X>0 && A.Y>0 && B.X>0 && B.Y>0 &&
                int64(Origin.X)<int64(Other.Origin.X)+B.X &&
                int64(Other.Origin.X)<int64(Origin.X)+A.X &&
                int64(Origin.Y)<int64(Other.Origin.Y)+B.Y &&
                int64(Other.Origin.Y)<int64(Origin.Y)+A.Y;
        }
    };

    inline bool CanPlace(const FPlacement& Candidate, FIntPoint MapSize,
        const TArray<FPlacement>& Occupied, const TArray<FIntPoint>& ReservedCells)
    {
        if (!Candidate.Fits(MapSize)) return false;
        for (const auto& Existing : Occupied)
            if (Candidate.Overlaps(Existing)) return false;
        for (const auto Cell : ReservedCells)
            if (Candidate.Contains(Cell)) return false;
        return true;
    }

    // Fit the unrotated local mesh bounds uniformly, preserving proportions.
    inline double UniformScale(EObject Object, double MeshX, double MeshY)
    {
        if (!FMath::IsFinite(MeshX) || !FMath::IsFinite(MeshY) || MeshX<=0 || MeshY<=0) return 0;
        const auto S=Footprint(Object);
        return FMath::Min(S.X*CellSize/MeshX,S.Y*CellSize/MeshY);
    }
}
