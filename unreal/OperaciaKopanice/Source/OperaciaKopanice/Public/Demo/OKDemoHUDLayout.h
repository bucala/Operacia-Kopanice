#pragma once
#include "CoreMinimal.h"

namespace OKHUD
{
    struct FLayout
    {
        FBox2D Officer, Partisan, Objective, Message;
        FVector2D Camera, Movement;
        float Button = 44;
    };
    inline FLayout Layout(float W, float H)
    {
        FLayout L;
        const bool Narrow=W<760;
        const float Card=Narrow ? 80.f : H>=900 ? 160.f : 128.f;
        const float CardHeight=Card+32;
        L.Officer=FBox2D(FVector2D(16,16),FVector2D(16+Card,16+CardHeight));
        const FVector2D P=Narrow ? FVector2D(24+Card,16) : FVector2D(16,24+CardHeight);
        L.Partisan=FBox2D(P,P+FVector2D(Card,CardHeight));
        const bool Portrait=Narrow && H>=640 && W<600;
        const float ObjectiveW=FMath::Min(Narrow ? Portrait ? 280.f : W-216 : 330.f,W-32);
        const float ObjectiveY=Portrait ? CardHeight+24 : 16;
        L.Objective=FBox2D(FVector2D(W-ObjectiveW-16,ObjectiveY),FVector2D(W-16,ObjectiveY+100));
        L.Movement=FVector2D(W-172,H-172);
        // Camera tools form two rows, to keep targets touch-sized on narrow views.
        L.Camera=FVector2D(W-232,H-280);
        const float MessageW=FMath::Min(420.f,W-204);
        L.Message=FBox2D(FVector2D(16,H-100),FVector2D(16+MessageW,H-16));
        return L;
    }
}
