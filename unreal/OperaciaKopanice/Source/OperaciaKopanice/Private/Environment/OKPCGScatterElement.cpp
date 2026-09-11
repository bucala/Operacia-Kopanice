#include "Environment/OKPCGScatterElement.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGPointArrayData.h"

void UOKPCGScatterElement::Execute_Implementation(const FPCGDataCollection& Input, FPCGDataCollection& Output)
{
    const UOKPCGScatterRuleSet* Rules = RuleSet ? RuleSet.Get() : GetDefault<UOKPCGScatterRuleSet>();
    for (const FPCGTaggedData& Tagged : Input.GetInputsByPin(TEXT("In")))
    {
        const auto* Points = Cast<UPCGBasePointData>(Tagged.Data);
        if (!Points) continue;
        TArray<int32> Accepted;
        TArray<FTransform> Transforms;
        const auto InTransforms = Points->GetConstTransformValueRange();
        const auto Densities = Points->GetConstDensityValueRange();
        const auto Seeds = Points->GetConstSeedValueRange();
        for (int32 I = 0; I < Points->GetNumPoints(); ++I)
        {
            FTransform Transform;
            if (Rules->EvaluatePoint(RuleIndex, InTransforms[I], Densities[I], Seeds[I] ^ Seed, AltitudeOffsetMeters, Transform))
            {
                Accepted.Add(I);
                Transforms.Add(Transform);
            }
        }
        auto* Result = NewObject<UPCGPointArrayData>();
        Result->InitializeFromData(Points);
        Result->SetNumPoints(Accepted.Num());
        Result->AllocateProperties(EPCGPointNativeProperties::All);
        for (int32 I = 0; I < Accepted.Num(); ++I) Points->CopyPointsTo(Result, Accepted[I], I, 1);
        auto OutTransforms = Result->GetTransformValueRange();
        auto OutDensities = Result->GetDensityValueRange();
        for (int32 I = 0; I < Accepted.Num(); ++I)
        {
            OutTransforms[I] = Transforms[I];
            OutDensities[I] = 1.0f;
        }
        FPCGTaggedData& Out = Output.TaggedData.Add_GetRef(Tagged);
        Out.Data = Result;
        Out.Pin = TEXT("Out");
    }
}
