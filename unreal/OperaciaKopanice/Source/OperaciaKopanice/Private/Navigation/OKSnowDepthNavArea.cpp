#include "Navigation/OKSnowDepthNavArea.h"

UOKSnowDepthNavArea::UOKSnowDepthNavArea()
{
	DefaultCost = 1.0f / 0.285f;
	FixedAreaEnteringCost = 0.0f;
	DrawColor = FColor(175, 210, 255);
}
