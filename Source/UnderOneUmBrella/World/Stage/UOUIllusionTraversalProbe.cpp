#include "World/Stage/UOUIllusionTraversalProbe.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Engine/World.h"
#if UOU_WITH_DEVELOPMENT_TOOLS
#include "Debug/UOUDevelopmentDebugDrawContext.h"
#endif

AUOUIllusionTraversalProbe::AUOUIllusionTraversalProbe()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = true;
	// 일반 이동이 끝난 뒤 가상 발 보행만 갱신한다. 일반 보행에는 개입하지 않는다.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AUOUIllusionTraversalProbe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if UOU_WITH_DEVELOPMENT_TOOLS
	LastDeltaSeconds = DeltaSeconds;
	UpdateProbe();
	TickVirtualWalking(DeltaSeconds);
	UpdateVirtualVisual();
#else
	SetActorTickEnabled(false);
#endif
}

float AUOUIllusionTraversalProbe::CalculateDepthGap(const FVector& ExpectedLocation, const FVector& SurfaceLocation, const FVector& ViewDirection)
{
	return FVector::DotProduct(SurfaceLocation - ExpectedLocation, ViewDirection.GetSafeNormal());
}

void AUOUIllusionTraversalProbe::SetResult(EUOUIllusionProbeStatus NewStatus, const TCHAR* Message)
{
	Status = NewStatus;
	Reason = Message;
}

bool AUOUIllusionTraversalProbe::TraceScreenPoint(APlayerController* Controller, ACharacter* Character,
	const FVector& WorldPoint, FHitResult& Hit) const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (TestScreenTrace) return TestScreenTrace(WorldPoint, Hit);
#endif
	FVector2D Pixel;
	if (!Controller->ProjectWorldLocationToScreen(WorldPoint, Pixel)) return false;
	int32 Width = 0, Height = 0;
	Controller->GetViewportSize(Width, Height);
	if (Pixel.X < 0 || Pixel.Y < 0 || Pixel.X >= Width || Pixel.Y >= Height) return false;
	FVector Origin, Direction;
	if (!Controller->DeprojectScreenPositionToWorld(Pixel.X, Pixel.Y, Origin, Direction)) return false;
	// 직교 카메라는 픽셀마다 시선 시작점이 다르므로 카메라 중심에서 쏘지 않는다.
	return TraceViewRay(Character, Origin, Direction, Hit);
}

bool AUOUIllusionTraversalProbe::TraceViewRay(ACharacter* Character, const FVector& Origin,
	const FVector& Direction, FHitResult& Hit) const
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IllusionScreenProbe), true, Character);
	Query.AddIgnoredActor(this);
	TArray<AActor*> AttachedActors;
	Character->GetAttachedActors(AttachedActors, true, true);
	Query.AddIgnoredActors(AttachedActors);
	return GetWorld()->LineTraceSingleByChannel(Hit, Origin,
		Origin + Direction * FMath::Max(100.0f, TraceDistance), TraceChannel, Query);
}

void AUOUIllusionTraversalProbe::UpdateProbe()
{
	// 이전 후보가 정지나 회전 후에도 유효하게 보이지 않도록 매번 결과를 초기화한다.
	CurrentPlatform = nullptr;
	CandidatePlatform = nullptr;
	bHasFeet = false;
	bHasCandidate = false;
	CandidateDepthGap = 0.0f;
	CandidateLocation = FVector::ZeroVector;
	CandidateNormal = FVector::ZeroVector;
	if (!bProbeEnabled)
	{
		bHasCameraHistory = false;
		SetResult(EUOUIllusionProbeStatus::Disabled, TEXT("진단 비활성"));
		return;
	}
	ACharacter* Character = IsValid(TargetCharacter) ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(this, 0);
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController() || !PC->PlayerCameraManager)
	{
		bHasCameraHistory = false;
		SetResult(EUOUIllusionProbeStatus::NoPlayer, TEXT("로컬 플레이어 카메라를 찾을 수 없음"));
		return;
	}
	const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
	if (View.ProjectionMode != ECameraProjectionMode::Orthographic)
	{
		bHasCameraHistory = false;
		SetResult(EUOUIllusionProbeStatus::NotOrthographic, TEXT("직교 카메라에서만 검사"));
		return;
	}
	auto* Movement = Character->GetCharacterMovement();
	const auto* Capsule = Character->GetCapsuleComponent();
	FeetLocation = Capsule->GetComponentLocation() - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();
	bHasFeet = true;
	CurrentPlatform = Movement->CurrentFloor.HitResult.GetActor();
	const auto* CameraController = Character->FindComponentByClass<UUOUCameraControllerComponent>();
	const bool bRotating = !bHasCameraHistory || PreviousCharacter.Get() != Character
		|| !PreviousCameraRotation.Equals(View.Rotation, 0.05f)
		|| (CameraController && CameraController->IsSnapCameraRotationInProgress());
	CameraStableTime = bRotating ? 0.0f : CameraStableTime + LastDeltaSeconds;
	PreviousCameraRotation = View.Rotation;
	PreviousCharacter = Character;
	bHasCameraHistory = true;
	if (CameraStableTime < 0.15f)
	{
		SetResult(EUOUIllusionProbeStatus::CameraMoving, TEXT("카메라 회전 중 또는 안정 대기"));
		return;
	}
	if (bVirtualWalking)
	{
		CurrentPlatform = VirtualSupport.IsValid() ? VirtualSupport->GetOwner() : nullptr;
		SetResult(EUOUIllusionProbeStatus::Candidate, TEXT("가상 발 보행: 현재 충돌 표면으로 지지면 유지"));
		return;
	}
	if (!Movement->IsMovingOnGround() || !Movement->CurrentFloor.IsWalkableFloor())
	{
		SetResult(EUOUIllusionProbeStatus::NotGrounded, TEXT("지상 보행 상태가 아님"));
		return;
	}
	if (!AllowedPlatforms.Contains(CurrentPlatform))
	{
		SetResult(EUOUIllusionProbeStatus::UnregisteredFloor, TEXT("현재 발판을 허용 목록에 등록해야 함"));
		return;
	}
	// 발 애니메이션 대신 충돌체 바닥을 사용하고, 현재 표면이 실제로 보이는지 확인한다.
	FHitResult FeetHit;
	const bool bVisible = TraceScreenPoint(PC, Character, FeetLocation, FeetHit)
		&& FeetHit.GetComponent() == Movement->CurrentFloor.HitResult.GetComponent()
		&& FVector::Dist(FeetHit.ImpactPoint, FeetLocation) <= 15.0f;
	if (!bVisible)
	{
		SetResult(EUOUIllusionProbeStatus::FeetOccluded, TEXT("발 기준점 가림: 뒤쪽 경로 유지 / 자동 이동 없음"));
		return;
	}
	const FVector Direction = Character->GetLastMovementInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		SetResult(EUOUIllusionProbeStatus::NoInput, TEXT("이동 입력 대기"));
		return;
	}
	// 첫 기준선은 전방 한 지점을 검사한다. 연속 경계·접근 이력 판정은 아직 확정하지 않는다.
	const FVector Expected = FeetLocation + Direction * FMath::Max(1.0f, ProbeDistance);
	FHitResult Hit;
	if (!TraceScreenPoint(PC, Character, Expected, Hit))
	{
		SetResult(EUOUIllusionProbeStatus::NoSurface, TEXT("전방 화면 위치에 충돌 표면 없음 또는 화면 밖"));
		return;
	}
	CandidatePlatform = Hit.GetActor();
	CandidateLocation = Hit.ImpactPoint;
	CandidateNormal = Hit.ImpactNormal;
	CandidateDepthGap = CalculateDepthGap(Expected, CandidateLocation, View.Rotation.Vector());
	bHasCandidate = true;
	if (Hit.GetComponent() == Movement->CurrentFloor.HitResult.GetComponent())
	{
		SetResult(EUOUIllusionProbeStatus::SameSurface, TEXT("같은 발판: 기존 보행 유지"));
		return;
	}
	if (!AllowedPlatforms.Contains(CandidatePlatform))
	{
		SetResult(EUOUIllusionProbeStatus::UnregisteredTarget, TEXT("도착 후보가 허용 발판이 아님"));
		return;
	}
	if (!Movement->IsWalkable(Hit))
	{
		SetResult(EUOUIllusionProbeStatus::NotWalkable, TEXT("벽면 또는 보행 불가 경사"));
		return;
	}
	if (FMath::Abs(CandidateDepthGap) < FMath::Max(1.0f, MinimumDepthGap))
	{
		SetResult(EUOUIllusionProbeStatus::NoDepthGap, TEXT("깊이 차이 작음: 일반 보행 우선"));
		return;
	}
	// 실제 발밑 공간은 레벨 설계에 맡긴다. 기준선도 가상 발 방식과 같은 후보만 표시한다.
	SetResult(EUOUIllusionProbeStatus::Candidate, TEXT("등록 충돌 표면 후보: 이동 판정 항목 확인"));
}

FText AUOUIllusionTraversalProbe::GetDebugDisplayName_Implementation() const
{
	return FText::FromString(TEXT("착시 이동 진단 — 충돌 기반"));
}

FText AUOUIllusionTraversalProbe::GetDebugSummaryText_Implementation() const
{
	return FText::FromString(FString::Printf(TEXT("%s\n현재: %s / 후보: %s\n시선 깊이 차이: %.1f cm\n이동 판정: %s\n자동 이동: %s"),
		*Reason, *GetNameSafe(CurrentPlatform), *GetNameSafe(CandidatePlatform), CandidateDepthGap,
		*TextureTraversalReason, bEnableTextureAutoTraversal ? TEXT("켜짐 · 실험") : TEXT("꺼짐")));
}

FVector AUOUIllusionTraversalProbe::GetDebugWorldLocation_Implementation() const
{
	return bHasFeet ? FeetLocation + FVector(0, 0, 150) : GetActorLocation();
}

#if UOU_WITH_DEVELOPMENT_TOOLS
void AUOUIllusionTraversalProbe::GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const
{
	if (!bProbeEnabled) return;
	if (bHasFeet) Context.DrawSphere(FeetLocation, 8, 12, FColor::Red, 1.5f);
	if (bHasTraversalBoundary)
	{
		Context.DrawSphere(BoundarySourcePoint, 6, 12, FColor::Orange, 2);
		Context.DrawSphere(BoundaryTargetPoint, 6, 12, FColor::Cyan, 2);
		Context.DrawLine(BoundarySourcePoint, BoundaryTargetPoint, FColor::Cyan, 1);
		Context.DrawString(BoundarySourcePoint + FVector(0, 0, 30),
			TEXT("주황: 출발면 끝 / 하늘색: 연결면 시작"), FColor::Orange, 1);
	}
	if (bHasFeet && !bHasVirtualPrediction && !LastProbeDirection.IsNearlyZero())
	{
		// 유효 표면을 찾지 못했을 때도 요청 위치는 보이되 성공 색상과 구분한다.
		const FVector Pending = FeetLocation + LastProbeDirection * FMath::Max(1.0f, ProbeDistance);
		Context.DrawSphere(Pending, 7, 12, FColor::Silver, 1);
		Context.DrawLine(FeetLocation, Pending, FColor::Silver, 1);
		Context.DrawString(Pending, TEXT("검사 요청점: 표면 확인 대기"), FColor::Silver, 1);
	}
	if (bHasVirtualPrediction)
	{
		Context.DrawSphere(CandidateLocation, 9, 12, FColor::Green, 2);
		Context.DrawSphere(PredictedFeetLocation, 9, 12, FColor::Magenta, 2);
		Context.DrawLine(PredictedFeetLocation, CandidateLocation, FColor::Green, 2);
		Context.DrawString(PredictedFeetLocation, TEXT("보라: 예상 발 / 초록: 가상 발 / 빨강: 실제 발"), FColor::White, 1);
	}
	if (bHasCandidate && !bHasVirtualPrediction)
	{
		const FColor Color = Status == EUOUIllusionProbeStatus::Candidate ? FColor::Yellow : FColor::Red;
		Context.DrawSphere(CandidateLocation, 10, 12, Color, 2);
		Context.DrawLine(FeetLocation, CandidateLocation, Color, 2);
		Context.DrawArrow(CandidateLocation, CandidateLocation + CandidateNormal * 60, 10, Color, 2);
	}

}
#endif
