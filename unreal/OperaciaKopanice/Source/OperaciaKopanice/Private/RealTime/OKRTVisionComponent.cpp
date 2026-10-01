#include "RealTime/OKRTVisionComponent.h"
#include "RealTime/OKRTUnit.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

UOKRTVisionComponent::UOKRTVisionComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
    SetCollisionProfileName(TEXT("NoCollision"));
    SetCanEverAffectNavigation(false);
    SetCastShadow(false);
}
void UOKRTVisionComponent::BeginPlay()
{
    Super::BeginPlay();
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Kopanice/WinterGround/M_TacticalOverlay.M_TacticalOverlay"));
    for (int32 I=0;I<2;++I)
    {
        auto* Material=UMaterialInstanceDynamic::Create(Base,this);
        if (!Material) continue;
        Material->SetScalarParameterValue(TEXT("GridVisibility"),0);
        Material->SetScalarParameterValue(TEXT("Fill"),I ? .10f : .23f);
        Material->SetVectorParameterValue(TEXT("Color"),I ? FLinearColor(.92f,.48f,.18f,.3f) : FLinearColor(.9f,.12f,.12f,.4f));
        SetMaterial(I,Material);
    }
}
bool UOKRTVisionComponent::Sees(const AOKRTUnit* Target,bool& bPrimary) const
{
    bPrimary=false;
    auto* Owner=Cast<AOKRTUnit>(GetOwner());
    if (!Owner || !Target || Target->bHiddenBody || Target->Carrier.IsValid()) return false;
    FVector Eye; FRotator Rotation;
    Owner->GetActorEyesViewPoint(Eye,Rotation);
    FVector TargetEye; FRotator Ignored;
    Target->GetActorEyesViewPoint(TargetEye,Ignored);
    FVector Delta=TargetEye-Eye;
    const float Distance=Delta.Size2D();
    if (Distance>FarRange || Distance<1) return false;
    const float Dot=FVector::DotProduct(Owner->GetActorForwardVector().GetSafeNormal2D(),Delta.GetSafeNormal2D());
    if (Dot<FMath::Cos(FMath::DegreesToRadians(HalfAngle))) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRTSight),false,Owner);
    Params.AddIgnoredActor(Target);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit,Eye,TargetEye,ECC_Visibility,Params)) return false;
    bPrimary=Distance<NearRange && Dot>FMath::Cos(FMath::DegreesToRadians(35.f));
    return true;
}
void UOKRTVisionComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    auto* Unit=Cast<AOKRTUnit>(GetOwner());
    SetVisibility(bConeVisible && Unit && Unit->IsAlive());
    Accumulator+=Delta;
    if (IsVisible() && Accumulator>.12f) { Accumulator=0; Refresh(); }
}
void UOKRTVisionComponent::Refresh()
{
    auto* Unit=Cast<AOKRTUnit>(GetOwner());
    if (!Unit) return;
    SetRelativeLocation(FVector(0,0,-Unit->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4));
    FVector Eye; FRotator Rotation;
    Unit->GetActorEyesViewPoint(Eye,Rotation);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OKRTCone),false,Unit);
    for (int32 Section=0;Section<2;++Section)
    {
        TArray<FVector> V; TArray<int32> Indices;
        TArray<FVector> Normals; TArray<FVector2D> UV; TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;
        constexpr int32 Segments=48;
        for (int32 I=0;I<=Segments;++I)
        {
            const float ZoneAngle=Section ? HalfAngle : FMath::Min(HalfAngle,35.f);
            const float Angle=FMath::DegreesToRadians(-ZoneAngle+2*ZoneAngle*I/Segments);
            const FVector Local(FMath::Cos(Angle),FMath::Sin(Angle),0);
            const FVector Direction=Unit->GetActorRotation().RotateVector(Local);
            FHitResult Hit;
            float Reach=FarRange;
            if (GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Direction*FarRange,ECC_Visibility,Params))
                Reach=FVector::Dist2D(Eye,Hit.ImpactPoint);
            const float Start=0;
            const float End=Section ? Reach : FMath::Min(NearRange,Reach);
            V.Add(Local*Start); V.Add(Local*End);
            UV.Add(FVector2D(0,0)); UV.Add(FVector2D(0,0));
            if (I<Segments)
            {
                const int32 A=I*2;
                Indices.Append({A,A+1,A+3,A,A+3,A+2});
            }
        }
        CreateMeshSection_LinearColor(Section,V,Indices,Normals,UV,Colors,Tangents,false);
    }
}
