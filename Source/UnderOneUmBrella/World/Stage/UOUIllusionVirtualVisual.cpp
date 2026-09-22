#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

void AUOUIllusionTraversalProbe::ResetVirtualVisual()
{
	// 이 기능이 추가한 상대 위치만 제거하여 캐릭터 고유의 메시 배치를 보존한다.
	if (auto* Mesh = VisualMesh.Get()) Mesh->SetRelativeLocation(Mesh->GetRelativeLocation() - AppliedVisualOffset);
	AppliedVisualOffset = FVector::ZeroVector;
	VisualDepthOffset = 0.0f;
	VisualMesh.Reset();
}

void AUOUIllusionTraversalProbe::UpdateVirtualVisual()
{
	ACharacter* Character = VirtualCharacter.Get();
	if (!bVirtualWalking || !bCorrectVirtualOcclusion || !Character)
	{
		ResetVirtualVisual();
		return;
	}
	auto* PC = Cast<APlayerController>(Character->GetController());
	auto* Mesh = Character->GetMesh();
	if (!PC || !PC->PlayerCameraManager || !Mesh || !Mesh->GetAttachParent()) { ResetVirtualVisual(); return; }
	const auto& View = PC->PlayerCameraManager->GetCameraCacheView();
	if (View.ProjectionMode != ECameraProjectionMode::Orthographic) { ResetVirtualVisual(); return; }
	if (VisualMesh.Get() != Mesh) { ResetVirtualVisual(); VisualMesh = Mesh; }
	const FVector Forward = View.Rotation.Vector();
	const auto* Capsule = Character->GetCapsuleComponent();
	const FVector Feet = Capsule->GetComponentLocation() - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();
	const FVector Side = FRotationMatrix(View.Rotation).GetUnitAxis(EAxis::Y) * Capsule->GetScaledCapsuleRadius() * 0.6;
	const FVector Samples[] = {Feet + FVector::UpVector * 5,
		Feet + FVector::UpVector * 25 + Side, Feet + FVector::UpVector * 25 - Side,
		Feet + FVector::UpVector * 50};
	float RequiredOffset = 0;
	for (const FVector& Point : Samples)
	{
		FHitResult Hit;
		if (TraceScreenPoint(PC, Character, Point, Hit) && Hit.GetComponent() == VirtualSupport.Get())
		{
			const float OccludingDepth = FVector::DotProduct(Point - Hit.ImpactPoint, Forward);
			if (OccludingDepth > 0) RequiredOffset = FMath::Max(RequiredOffset, OccludingDepth + 15.0f);
		}
	}
	// 직교 시선 방향만 이동하므로 화면상의 크기와 발 위치는 유지한다. 논리 위치와 도착 검사는 변경하지 않는다.
	// 가림 방지는 즉시 적용하고, 가림 해제는 서서히 복귀시켜 경계에서 보정이 반복해서 튀지 않게 한다.
	const float TargetDepth = FMath::Clamp(RequiredOffset, 0.0f, FMath::Max(0.0f, MaximumVisualDepthOffset));
	VisualDepthOffset = TargetDepth >= VisualDepthOffset ? TargetDepth
		: FMath::FInterpConstantTo(VisualDepthOffset, TargetDepth, FMath::Max(0.0f, LastDeltaSeconds), 2000.0f);
	const FVector WorldOffset = -Forward * VisualDepthOffset;
	const FVector RelativeOffset = Mesh->GetAttachParent()->GetComponentTransform().InverseTransformVector(WorldOffset);
	Mesh->SetRelativeLocation(Mesh->GetRelativeLocation() - AppliedVisualOffset + RelativeOffset);
	AppliedVisualOffset = RelativeOffset;
}
