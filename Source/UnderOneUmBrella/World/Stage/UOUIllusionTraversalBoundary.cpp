#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool AUOUIllusionTraversalProbe::IsEntryBoundaryReached(float TravelDistance) const
{
	// 후보를 찾는 거리와 실제 진입 거리는 별개다. 다음 이동 구간이 경계에 닿을 때만 진입한다.
	return bHasTraversalBoundary && BoundaryTravelDistance <= FMath::Max(0.0f, TravelDistance)
		+ FMath::Clamp(EntryBoundaryTolerance, 0.0f, 10.0f);
}

void AUOUIllusionTraversalProbe::UpdateTraversalBoundary(APlayerController* Controller,
	ACharacter* Character, UPrimitiveComponent* Source, const FVector& Feet, const FVector& Expected)
{
	bHasTraversalBoundary = false;
	BoundaryTargetHit = FHitResult();
	BoundaryTravelDistance = 0.0f;
	FHitResult Previous;
	if (!TraceScreenPoint(Controller, Character, Feet, Previous) || Previous.GetComponent() != Source) return;
	// 검사 거리 전체를 한 번에 건너뛰지 않고 처음 표면이 바뀌는 구간을 찾는다.
	constexpr int32 Segments = 16;
	float Start = 0;
	for (int32 Index = 1; Index <= Segments; ++Index)
	{
		float End = static_cast<float>(Index) / Segments;
		FHitResult Next;
		if (!TraceScreenPoint(Controller, Character, FMath::Lerp(Feet, Expected, End), Next)) return;
		// 벽이나 보행 불가 면을 건너뛰어 뒤의 윗면을 연결하지 않는다.
		if (!Character->GetCharacterMovement()->IsWalkable(Next)) return;
		if (Next.GetComponent() == Source)
		{
			Start = End;
			Previous = Next;
			continue;
		}
		// 등록되지 않은 표면이나 빈 공간은 연결 경계로 표시하지 않는다.
		if (!AllowedPlatforms.Contains(Next.GetActor())) return;
		UPrimitiveComponent* Target = Next.GetComponent();
		for (int32 Refine = 0; Refine < 8; ++Refine)
		{
			const float Middle = (Start + End) * 0.5f;
			FHitResult Sample;
			if (!TraceScreenPoint(Controller, Character, FMath::Lerp(Feet, Expected, Middle), Sample)) return;
			if (!Character->GetCharacterMovement()->IsWalkable(Sample)) return;
			if (Sample.GetComponent() == Source) { Start = Middle; Previous = Sample; }
			else if (Sample.GetComponent() == Target) { End = Middle; Next = Sample; }
			else return;
		}
		BoundarySourcePoint = Previous.ImpactPoint;
		BoundaryTargetPoint = Next.ImpactPoint;
		BoundaryTargetHit = Next;
		BoundaryTravelDistance = FVector::Dist(Feet, Expected) * End;
		bHasTraversalBoundary = true;
		return;
	}
}
