#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

bool AUOUIllusionTraversalProbe::CalculateScreenPreservingFeet(const FVector& ExpectedFeet,
	const FVector& PlanePoint, const FVector& Normal, const FVector& ViewDirection,
	float FloorClearance, FVector& OutFeet)
{
	const FVector Ray = ViewDirection.GetSafeNormal();
	const FVector N = Normal.GetSafeNormal();
	const double Denominator = FVector::DotProduct(Ray, N);
	if (FMath::Abs(Denominator) < 0.001 || ExpectedFeet.ContainsNaN() || PlanePoint.ContainsNaN()) return false;
	// 동일 픽셀의 시선과 바닥 여유만큼 올린 평면을 교차시킨다. 화면 수평·수직 성분은 변하지 않는다.
	const FVector RaisedPlane = PlanePoint + FVector::UpVector * FloorClearance;
	OutFeet = ExpectedFeet + Ray * (FVector::DotProduct(RaisedPlane - ExpectedFeet, N) / Denominator);
	return !OutFeet.ContainsNaN();
}

bool AUOUIllusionTraversalProbe::FindAscendingSurface(APlayerController* Controller, ACharacter* Character,
	UPrimitiveComponent* Source, const FVector& Feet, const FVector& Direction, const FVector& ReferenceNormal,
	float& OutDistance, FVector& OutExpected, FHitResult& OutHit) const
{
	// 끝점이 윗면을 넘어선 경우에만 검사 구간 안의 상승 후보를 찾는다. 검사 거리는 늘리지 않는다.
	const FVector ViewDirection = Controller->PlayerCameraManager->GetCameraRotation().Vector();
	for (int32 Index = 15; Index >= 1; --Index)
	{
		const float Distance = FMath::Max(1.0f, ProbeDistance) * Index / 16.0f;
		const FVector Expected = Feet + Direction * Distance;
		FHitResult Hit;
		if (!TraceScreenPoint(Controller, Character, Expected, Hit) || Hit.GetComponent() == Source
			|| !AllowedPlatforms.Contains(Hit.GetActor()) || !Character->GetCharacterMovement()->IsWalkable(Hit)
			|| Hit.ImpactPoint.Z <= Feet.Z + 5.0f
			|| FVector::DotProduct(ReferenceNormal, Hit.ImpactNormal.GetSafeNormal()) < NormalAgreement
			|| FMath::Abs(CalculateDepthGap(Expected, Hit.ImpactPoint, ViewDirection)) < MinimumDepthGap) continue;
		OutDistance = Distance;
		OutExpected = Expected;
		OutHit = Hit;
		return true;
	}
	return false;
}
