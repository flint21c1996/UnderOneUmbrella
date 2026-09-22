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
