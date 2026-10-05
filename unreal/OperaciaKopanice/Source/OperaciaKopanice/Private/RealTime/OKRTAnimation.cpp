#include "RealTime/OKRTUnit.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"

void AOKRTUnit::UpdateAnimation(float Delta)
{
    const float FrameDelta=FMath::Clamp(Delta,0.f,.05f);
    SmoothedSpeed=FMath::FInterpTo(SmoothedSpeed,FMath::Min(GetVelocity().Size2D(),360.f),FrameDelta,8);
    const float Speed=SmoothedSpeed;
    MotionBlend=FMath::FInterpTo(MotionBlend,FMath::Clamp(Speed/90.f,0.f,1.f),Delta,9);
    CrouchBlend=FMath::FInterpTo(CrouchBlend,Stance==EOKStance::Crouch ? 1.f : 0.f,Delta,8);
    ProneBlend=FMath::FInterpTo(ProneBlend,Stance==EOKStance::Prone ? 1.f : 0.f,Delta,8);
    const float Stride=FMath::Lerp(145.f,65.f,FMath::Max(CrouchBlend,ProneBlend));
    const float CyclesPerSecond=FMath::Min(Speed/Stride,1.65f);
    GaitPhase=FMath::Fmod(GaitPhase+CyclesPerSecond*FrameDelta*2*PI,2*PI);
    const float Swing=FMath::Sin(GaitPhase)*MotionBlend;
    Visual->SetRelativeLocation(FVector(-75*ProneBlend,0,
        -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3+24*ProneBlend));
    Visual->SetRelativeRotation(FRotator(85*ProneBlend,0,0));
    auto* Rig=Cast<USkeletalMesh>(AnimatedVisual->GetSkinnedAsset());
    if (!Rig) return;
    const auto& Ref=Rig->GetRefSkeleton();
    const auto& Bind=Ref.GetRefBonePose();
    TArray<FTransform,TInlineAllocator<32>> Rest,Pose;
    Rest.SetNum(Bind.Num()); Pose.SetNum(Bind.Num());
    const float Run=FMath::Clamp((Speed-190)/170.f,0.f,1.f);
    const float Amplitude=FMath::Lerp(14.f,24.f,Run)*(1-.55f*ProneBlend);
    for (int32 I=0;I<Bind.Num();++I)
    {
        const int32 Parent=Ref.GetParentIndex(I);
        Rest[I]=Parent==INDEX_NONE ? Bind[I] : Bind[I]*Rest[Parent];
        FTransform Local=Bind[I];
        const FString Name=Ref.GetBoneName(I).ToString();
        float Angle=0;
        const float Side=Name.EndsWith(TEXT("_l")) ? 1.f : -1.f;
        if (Name.StartsWith(TEXT("thigh_"))) Angle=Side*Swing*Amplitude-32*CrouchBlend;
        else if (Name.StartsWith(TEXT("calf_"))) Angle=FMath::Max(0.f,-Side*Swing)*32+62*CrouchBlend;
        else if (Name.StartsWith(TEXT("foot_"))) Angle=-20*CrouchBlend;
        else if (Name.StartsWith(TEXT("arm_"))) Angle=-Side*Swing*Amplitude*.65f-25*ProneBlend;
        else if (Name.StartsWith(TEXT("forearm_"))) Angle=-12*MotionBlend-25*Run;
        else if (Name==TEXT("chest")) Angle=-12*CrouchBlend-6*Run*MotionBlend;
        else if (Name==TEXT("head")) Angle=8*CrouchBlend;
        // Rotate in the character sagittal plane despite FBX bone-axis conventions.
        const FVector Axis=Rest[I].GetRotation().Inverse().RotateVector(FVector::YAxisVector);
        Local.SetRotation((Local.GetRotation()*FQuat(Axis,FMath::DegreesToRadians(Angle))).GetNormalized());
        Pose[I]=Parent==INDEX_NONE ? Local : Local*Pose[Parent];
        // FBX parents may carry a 100x unit scale and rotated axes. Apply centimetre
        // offsets in component space, never in the imported bone's local space.
        if (Name==TEXT("pelvis"))
            Pose[I].AddToTranslation(FVector(0,0,-23*CrouchBlend+
                (1-FMath::Cos(2*GaitPhase))*.3f*MotionBlend));
    }
    for (int32 I=0;I<Bind.Num();++I)
        AnimatedVisual->SetBoneTransformByName(Ref.GetBoneName(I),Pose[I],EBoneSpaces::ComponentSpace);
}
