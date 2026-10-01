#include "Demo/OKDemoGameMode.h"
#include "Environment/OKWinterEnvironmentActor.h"

void AOKDemoGameMode::BuildWinterTree(FVector Origin,int32 Seed,float Height)
{
    if (Environment) Environment->BuildWinterTree(Origin,Seed,Height);
}
